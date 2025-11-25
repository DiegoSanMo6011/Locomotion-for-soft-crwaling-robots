%% ================================================================
%  analisis_escalon_bomba_ms.m  (versión en milisegundos)
%
%  Analiza un experimento de escalón en presión negativa o positiva.
%  Entrada CSV:
%       t_ms,PWM,Presion_kPa
%
%  Calcula:
%    - u0, u1 (antes y después del escalón)
%    - y0, y_inf
%    - K (ganancia estática)
%    - tau en MILISEGUNDOS
%
%  ================================================================

clear; clc; close all;

%% 1) Configuración
nombreArchivo = 'datos_barrido_REP.csv';

% Ventanas (ahora expresadas en MILISEGUNDOS)
tiempo_baseline_ms = 2000;   % 2 s ANTES del escalón
tiempo_final_ms    = 5000;   % 5 s al final
umbralPWM          = 5;      % diferencia para detectar escalón


%% 2) Cargar datos
data = readtable(nombreArchivo);

t_ms  = data.t_ms;
t_s   = t_ms / 1000;
u_pwm = data.PWM;
y_kPa = data.Presion_kPa;

N = numel(t_ms);
fprintf('Se cargaron %d muestras del archivo %s\n', N, nombreArchivo);


%% 3) Gráfica general
figure('Name','Escalón medido - Señales completas');
subplot(2,1,1);
plot(t_s, u_pwm, 'LineWidth', 1); grid on;
xlabel('Tiempo [s]'); ylabel('PWM'); title('Entrada PWM');

subplot(2,1,2);
plot(t_s, y_kPa, 'LineWidth', 1); grid on;
xlabel('Tiempo [s]'); ylabel('Presión [kPa]'); title('Presión medida');


%% 4) Detectar el instante del escalón
% Usamos primeras muestras (hasta 1500 ms) para estimar PWM inicial
idx_iniVentana = t_ms <= 1500;
u0_est = mean(u_pwm(idx_iniVentana));

idx_cambio = find( abs(u_pwm - u0_est) > umbralPWM, 1, 'first' );

if isempty(idx_cambio)
    error('No se detectó un escalón claro en PWM');
end

t_step_ms = t_ms(idx_cambio);
fprintf('Escalón detectado en t = %.0f ms (muestra #%d)\n', t_step_ms, idx_cambio);


%% 5) Calcular u0, y0, u1, y_inf (usando ventanas en ms)
% Baseline ANTES del escalón
idx_baseline = (t_ms >= (t_step_ms - tiempo_baseline_ms)) & (t_ms < t_step_ms);
if ~any(idx_baseline)
    idx_baseline = t_ms < t_step_ms;
end

u0 = mean(u_pwm(idx_baseline));
y0 = mean(y_kPa(idx_baseline));

% Final del experimento (últimos X ms)
t_fin_ms = t_ms(end);
idx_final = t_ms >= (t_fin_ms - tiempo_final_ms);

u1 = mean(u_pwm(idx_final));
y_inf = mean(y_kPa(idx_final));

fprintf('u0 = %.2f, u1 = %.2f\n', u0, u1);
fprintf('y0 = %.3f kPa, y_inf = %.3f kPa\n', y0, y_inf);


%% 6) Ganancia estática
delta_u = u1 - u0;
delta_y = y_inf - y0;

if abs(delta_u) < 1e-6
    error('Delta_u ~ 0, no se puede calcular la ganancia');
end

K = delta_y / delta_u;
fprintf('Ganancia estática K = %.4f kPa/PWM\n', K);


%% 7) Calcular tau usando el 63% del cambio (válido para subida o bajada)
y63 = y0 + 0.632 * delta_y;

idx_despues = find(t_ms >= t_step_ms);
t_after  = t_ms(idx_despues);
y_after  = y_kPa(idx_despues);

% Fracción del cambio (escala de 0→1 independientemente del signo)
frac = (y_after - y0) ./ delta_y;

% Primer instante donde se llega al 63.2%
idx_tau_rel = find(frac >= 0.632, 1, 'first');

if isempty(idx_tau_rel)
    warning('No se encontró cruce del 63%%. Tau = NaN');
    tau_ms = NaN;
else
    t_tau_ms = t_after(idx_tau_rel);
    tau_ms = t_tau_ms - t_step_ms;
    fprintf('Constante de tiempo tau = %.2f ms\n', tau_ms);
end


%% 8) Gráfica detallada
figure('Name','Análisis del escalón - tau en ms');
plot(t_ms, y_kPa, 'b-', 'LineWidth', 1.2); hold on; grid on;

yline(y0,    '--k', 'y_0');
yline(y_inf, '--k', 'y_{\infty}');
yline(y63,   '--r', '63% del cambio');

xline(t_step_ms, '--g', 't_{step}');

if ~isnan(tau_ms)
    xline(t_step_ms + tau_ms, '--m', sprintf('t_{step} + tau = %.0f ms', t_step_ms + tau_ms));
end

xlabel('Tiempo [ms]');
ylabel('Presión [kPa]');
title('Respuesta al escalón y cálculo de \tau (ms)');

legend('Presión','y_0','y_\infty','y_{63%}','t_{step}','t_{step} + tau','Location','best');


%% 9) Resumen final
fprintf('\n===== RESUMEN DEL MODELO IDENTIFICADO =====\n');
fprintf('  K   = %.4f kPa/PWM\n', K);
fprintf('  tau = %.2f ms\n', tau_ms);
fprintf('  u0  = %.2f → u1 = %.2f\n', u0, u1);
fprintf('  y0  = %.3f → y_inf = %.3f kPa\n', y0, y_inf);
fprintf('===========================================\n');

