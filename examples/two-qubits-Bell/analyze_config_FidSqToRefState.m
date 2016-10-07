
dat = load('thedata.mat');

% -------------------

%fit_fun2_logp = @(a1, a2, m, c, x) -a1.*(1-x) - a2.*(1-x).^2 + m.*log(1-x) + c;
%fitopt2 = {'Lower', [-Inf, 0, 0, -Inf], ...
%           'StartPoint', [100, 1000, 0, 1] };

% fit_fun2P_logp = @(a, b, m, c, x)  -(((1-x) - b)/a).^2 + m.*log(1-x) + c;
% fitopt2P = {'Lower', [0, -Inf, 0, -Inf], ...
%            'StartPoint', [0.0378, -0.6593, 0, 1] };

ad = analyze_tomorun_histogram(...
    'tomorun-config7f2tgt-histogram.csv', 'FitThresFrac', 1e-3 ...
    , 'XIsOneMinus' ...
    ...%, 'IgnoreStartPoints', 15 ...
    ...% , 'PlotFitFnRes', 1000 ...
    ...%    , 'FitModel', fit_fun2_logp, 'FitWhich', 'LogP', 'FitOptions', fitopt2 ...
    );


figure(ad.FigHandleP);
set(gcf, 'WindowStyle', 'normal');
%set(gca, 'XLim', [1.6, 2]);
%set(gca, 'YLim', [0, 16]);
set(gcf,'PaperPositionMode','auto');
figpos = get(gcf, 'Position');
set(gcf, 'Position', [figpos(1) figpos(2) 320 240]);
xlabel('F^2(\rho, |\Psi\rangle\langle\Psi|)');
ylabel('normalized probability density');
legend('off');%legend({'histogram data', 'fit theoretical model'})
figname = get(gcf, 'Name');
export_fig([figname '.pdf'], '-transparent');

