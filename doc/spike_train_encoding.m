close all;
clear all;
more off;

alpha = 0.334335;
sigma = 0.0016875;
window_fun_exp = @(x, mu) alpha * (x >= mu) .* exp(-(x - mu) / sigma);
window_fun_gauss = @(x, mu) alpha * exp(-(x - mu).^2 / sigma.^2);
window_fun = window_fun_gauss;

integral = sum(window_fun(-1:1e-4:1, 0)) * 1e-4
integral0 = sum(window_fun(-1:1e-4:0, 0)) * 1e-4

resolution = 1e-4
min_spike_interval = 1e-3
xs = 0:resolution:2;
signal = (-cos(xs * 10 * 2 * pi) + 1) * 0.25 + 0.5;

# Delta-Sigma-Encoder
spikes = [];
approx = signal * 0;
err = 0;
errs = signal * 0;
last_spike = 0
for i = 1:numel(xs)
	err += (signal(i) - approx(i)) * resolution;
	if (err > integral) && ((i * resolution - last_spike) > min_spike_interval)
		spikes = [spikes, xs(i)];
		approx += window_fun(xs, xs(i));
		err -= integral0;
		last_spike = i * resolution;
	end
	errs(i) = err;
end

filtered = sum(window_fun(xs, repmat(spikes', 1, numel(xs))));
rmse = sqrt(sum((filtered - signal) .^ 2) / numel(signal))

subplot(2, 1, 1)
hold on
title("Input Spikes")
for spike = spikes
	plot([spike, spike], [-1, 1], 'k');
end
xlim([min(xs), max(xs)])

subplot(2, 1, 2);
hold on;
title("Filtered");
plot(xs, filtered, 'k-');
plot(xs, signal, 'k--');
xlim([min(xs), max(xs)])


