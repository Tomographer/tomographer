
dat = load('thedata.mat');

% -------------------

ad = analyze_tomorun_histogram(...
    'tomorun-config-TrDistToMLE-histogram.csv', ...
    'FitThresFrac', 1e-4, ...
    'XIsOneMinus', false, ...
    'NoPlotFitPoints' ...
    );

%
% Just plot the histogram & fit
%
figure(ad.FigHandleP);
set(gcf, 'WindowStyle', 'normal');
set(gca, 'XLim', [0, 0.2]);
set(gcf,'PaperPositionMode','auto');
figpos = get(gcf, 'Position');
set(gcf, 'Position', [figpos(1) figpos(2) 320 240]);
xlabel('trace distance to MLE estimate');
ylabel('normalized probability density');
%legend({'histogram data', 'fit theoretical model'})
figname = get(gcf, 'Name');

% To export the figure to PDF, you can either use MATLAB's built-in export tools, or
% install "export_fig":
% https://www.mathworks.com/matlabcentral/fileexchange/23629-export-fig
%
%export_fig([figname '.pdf'], '-transparent');



%
% also a log plot to inspect the fit quality
%
figure(ad.FigHandleLogP);
set(gcf, 'WindowStyle', 'normal');
set(gca, 'XLim', [0, 0.11]);
set(gcf,'PaperPositionMode','auto');
figpos = get(gcf, 'Position');
set(gcf, 'Position', [figpos(1) figpos(2) 320 240]);
xlabel('trace distance to MLE estimate');
ylabel('normalized probability density');
legend({'histogram data', 'fit theoretical model'}, 'Location', 'south')
axLog = gca;
axLog_pos = get(axLog, 'Position');
axLogVal = axes('Position', axLog_pos, ...
                'XTick', [], ...
                'XAxisLocation', 'top', ...
                'YAxisLocation', 'right', ...
                'YLimMode', 'manual', ...
                'YLim', log(get(axLog, 'YLim')), ...
                'Color', 'none');
ylabel(axLogVal, 'ln(\mu(f))')
figname = get(gcf, 'Name');

% To export the figure to PDF, you can either use MATLAB's built-in export tools, or
% install "export_fig":
% https://www.mathworks.com/matlabcentral/fileexchange/23629-export-fig
%
%export_fig([figname '.pdf'], '-transparent');



%
% Also provide a residuals plot, just for fun
%
mynamedfigure('figlogp-tomorun-config5td-residuals');
clf;
%plot(ad.thefit, ad.FitDataX, ad.FitDataY, 'b-x', 'residuals')
residuals = ad.FitDataY - ad.thefit(ad.FitDataX);
errorbar(ad.FitDataX, residuals, ad.FitDataErrors, 'LineStyle', 'none');
hold on;
plot([min(ad.FitDataX), max(ad.FitDataX)], [0 0], 'r-');
set(gca, 'YAxisLocation', 'right')
set(gca, 'XLim', [0 0.11]);%[min(ad.FitDataX) max(ad.FitDataX)]);
set(gca, 'YLim', [-.5, .5]);
set(gcf, 'WindowStyle', 'normal');
set(gcf,'PaperPositionMode','auto');
figpos = get(gcf, 'Position');
set(gcf, 'Position', [figpos(1) figpos(2) 320 120]);
xlabel('trace distance to MLE estimate');
ylabel('Residuals');
legend('off');
figname = get(gcf, 'Name');

% To export the figure to PDF, you can either use MATLAB's built-in export tools, or
% install "export_fig":
% https://www.mathworks.com/matlabcentral/fileexchange/23629-export-fig
%
%export_fig([figname '.pdf'], '-transparent');
