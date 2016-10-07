
dat = load('thedata.mat');

% -------------------

ad = analyze_tomorun_histogram(...
    'tomorun-config-EntglWitness-histogram.csv', ...
    'FitThresFrac', 1e-3, ...
    'FToX', [2, -1]);

%
% Plot
%
figure(ad.FigHandleP);
set(gcf, 'WindowStyle', 'normal');
set(gca, 'XLim', [1.6, 2]);
set(gca, 'YLim', [0, 16]);
set(gcf,'PaperPositionMode','auto');
figpos = get(gcf, 'Position');
set(gcf, 'Position', [figpos(1) figpos(2) 320 240]);
xlabel('entanglement witness \langle{W}\rangle');
ylabel('normalized probability density');
legend('off');%legend({'histogram data', 'fit theoretical model'})
figname = get(gcf, 'Name');

% To export the figure to PDF, you can either use MATLAB's built-in export tools, or
% install "export_fig":
% https://www.mathworks.com/matlabcentral/fileexchange/23629-export-fig
%
%export_fig([figname '.pdf'], '-transparent');

