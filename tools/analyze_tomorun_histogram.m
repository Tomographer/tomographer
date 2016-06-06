function dat = analyze_tomorun_histogram(varargin)
%
% Analyze a histogram produced by tomorun C++ integrator
% 
% dat = analyze_tomorun_histogram(histogramfilename, ...)
%
% Run analyze_tomorun_histogram('Help') for more information.
%
%
% This file is part of the Tomographer project, which is distributed under the
% terms of the MIT license. (see LICENSE.txt)


  optdefs = struct;
  optdefs.opt_allow_args = true;
  optdefs.opt_args_check = 1;
  optdefs.FitThresFrac = struct('arg', 'n', 'default', 1e-3);
  optdefs.FitThresFrac.help = ['Fraction of maximum value above which to consider points ' ...
                      'for fit. Points below this threshold will be ignored.'];
  optdefs.IgnoreStartPoints = struct('arg', 'n', 'default', 0);
  optdefs.IgnoreStartPoints.help = ['A number of points which should be ignored, starting ' ...
                      'from the f=0 side.'];
  optdefs.IgnoreEndPoints = struct('arg', 'n', 'default', 0);
  optdefs.IgnoreEndPoints.help = ['A number of points which should be ignored, starting ' ...
                      'from the f=max side.'];
  optdefs.CenterBins = struct('switch', true);
  optdefs.CenterBins.help = ['tomorun produces figure of merit values which correspond to lower ' ...
                      'bin values. Use this option to center the bins, i.e. move the x ' ...
                      'axis so that the x label corresponds to the center of a bin, and ' ...
                      'not the lower limit.'];
  %  optdefs.XIsOneMinus = struct('switch', false);
  %  optdefs.XIsOneMinus.help = ['If you set this option to true, then the X data is ' ...
  %                      'calculated as 1 - (x-value). This makes sense for example for the ' ...
  %                      'fidelity, which lies close to 1.'];
  optdefs.XIsOneMinus = struct('switch', false, 'imply', {{ 'FToX', [1, -1] }}, ...
                               'imply_if', @(tf) tf);
  optdefs.FToX = struct('arg', 'n', 'default', []);
  optdefs.FToX.help = ['Specify how to transform the figure of merit value f into the x ' ...
                      'coordinate for fit. Specify the transformation as a pair of ' ...
                      'values [h, s] in the relation x=s(f-h) or f=sx+h, where h can ' ...
                      'be any constant, and where s must be ' ...
                      'plus or minus one. By default, or if you specify an empty array' ...
                      'here, there is no transformation, x=f. For the fidelity, you ' ...
                      'should use x=1-f ("[1,-1]") or the "XIsOneMinus" option. For an ' ...
                      'entanglement witness, you might use x=2-f ("[2,-1]") for example.'];
  optdefs.FigHandleP = struct(...
      'arg', 'any', 'default', [] ...
      );
  optdefs.FigHandleP.help = ['Figure to use for plotting the probability density. Specify ' ...
                      'a figure handle, a name for mynamedfigure(), leave empty for ' ...
                      'default, or specify -1 to not produce figures.'];
  optdefs.FigHandleLogP = struct(...
      'arg', 'any', 'default', [] ...
      );
  optdefs.FigHandleLogP.help = ['Figure to use for plotting the logarithm of the ' ...
                      'probability density. Specify a figure handle, a name for ' ...
                      'mynamedfigure(), leave empty for default, or specify -1 to not ' ...
                      'produce figures.'];
  optdefs.ClearFigures = struct('switch', true);
  optdefs.ClearFigures.help = ['If set, then a figure will be cleared of any previous ' ...
                      'content before drawing to it (the default).'];
  optdefs.PlotFitFnRes = struct('arg', 'n', 'default', 200);
  optdefs.PlotFitFnRes.help = ['When plotting the fitted function, the number of points ' ...
                      'to use to display the fitted function.'];
  optdefs.PlotFitPoints = struct('switch', true);
  optdefs.PlotFitPoints.help = ['Display with red crosses which points where used for ' ...
                      'the fit, in the log(p) plot'];
  optdefs.PlotXNotF = struct('switch', false);
  optdefs.PlotXNotF.help = ['Plot against the x value (=h-s*f), not against f. See option ' ...
                      '"FToX".'];
  optdefs.QuErrorBars = struct('switch', true, 'imply', {{'PlotDeskewedGaussian', false}}, ...
                             'imply_if', @(v) ~v);
  optdefs.QuErrorBars.help = ['Calculate the quantum error bars corresponding to the fit ' ...
                      'parameters. Not supported if you provide your custom fit ' ...
                      'model.'];
  optdefs.PlotDeskewedGaussian = struct('switch', true);
  optdefs.PlotDeskewedGaussian.help = ['Plot also the de-skewed Gaussian corresponding ' ...
                      'to the quantum error bars without the m parameter.'];
  optdefs.CustomFit = struct('switch', false, 'imply', {{'QuErrorBars', false}}, ...
                             'imply_if', @(v) v);
  optdefs.CustomFit.help = ['Set to true if you have a custom model to fit. (This is ' ...
                      'implied by ''FitModel'', ''FitWhich'' or ''FitOptions''.)'];
  optdefs.FitModel = struct('arg', 'fh*', 'default', [], ...
                            'imply', {{'CustomFit'}});
  optdefs.FitModel.help = ['Specify a model to fit to. This should be a MATLAB function ' ...
                      'handle which can be provided to fit().'];
  optdefs.FitWhich = struct('arg', 's*', 'default', [], ...
                            'validator', validator_enum('LogP', 'P'), ...
                            'imply', {{'CustomFit'}});
  optdefs.FitWhich.help = ['Which of the data series P or LogP to fit to. Possible ' ...
                      'values are ''P'' or ''LogP'''];
  optdefs.FitOptions = struct('arg', 'cell', 'default', {{}}, 'imply', ...
                              {{'CustomFit'}});
  optdefs.FitOptions.help = ['A cell array of options which will be forwarded as is to ' ...
                      'fit(). Use this, for example, to specify limits or starting points.'];

  [opts, args] = parse_opts(optdefs, varargin);
  
  histfname = args{1};
  
  opts.FigHandleP = get_figure_handle(opts.FigHandleP, histfname, 'p');
  opts.FigHandleLogP = get_figure_handle(opts.FigHandleLogP, histfname, 'logp');
  
  data = importdata(histfname);
  
  F = data.data(:,1);
  AvgCounts = data.data(:,2);
  Error = data.data(:,3);
  
  if (opts.CenterBins)
    F = F + (F(2)-F(1)) ./ 2.0;
  end

  if (opts.FToX)
    h = opts.FToX(1);
    s = opts.FToX(2);
    assert(abs(s) == 1, 'The s constant in "FToX" must be plus or minus one!');
    XX = s*(F-h);
  else
    h = 0;
    s = 1;
    XX = F;
  end
  ftox = @(f) s*(f-h);
  xtof = @(x) s*x + h;

  if (any(XX==0))
    warning('analyze_tomorun_histogram:zeroInOneMinusFidelity', ['Found a zero value in ' ...
                        'X (transformed figure of merit). fit might complain about model computing Inf.']);
  end

  % Normalize to probability density
  histnorm = trapz(s*XX,AvgCounts);
  P = AvgCounts/histnorm;
  ErrorP = Error/histnorm;
  
  % get the "support" of P -- where P != 0
  P_IndNonZero = find(P>0);
  
  if (~isequal(opts.FigHandleLogP, -1) || ~isequal(opts.FigHandleP, -1))
    % for plots: some common defs (range of x's, and function to transform x back to f)
    xxmin = min(XX(P_IndNonZero));
    xxmax = max(XX(P_IndNonZero));
    xxpts = linspace(xxmin, xxmax, opts.PlotFitFnRes);
    plotx_trans = xtof;
    if (opts.PlotXNotF)
      plotx_trans = @(x) x;
    end
  end
  
  if (~isequal(opts.FigHandleP, -1))
    figure(opts.FigHandleP);
    if (opts.ClearFigures)
      clf;
    end
    if (opts.PlotXNotF)
      myplot_x = XX;
    else
      myplot_x = F;
    end
    errorbar(myplot_x, P, ErrorP, ...
             'Marker', '.', 'LineStyle', 'none');
    set(gca, 'YScale', 'linear')
  end

  % calculate logarithm of P
  LogP = log(P);
  ErrorLogP = ErrorP ./ P; % delta(log x) = delta(x) / x

  % --- fit model ---
  if (opts.CustomFit)
    assert(~isempty(opts.FitModel), 'No Fit Model provided (''FitModel'' option).');
    assert(~isempty(opts.FitWhich), ['You need to specify which data to fit to (''FitWhich''= ' ...
                        '''P''/''LogP'')']);
    thefitfunc = opts.FitModel;
    if (strcmp(opts.FitWhich, 'LogP'))
      fitlogp = true;
    else
      fitlogp = false;
    end
    fitoptions = opts.FitOptions;
  else
    fitlogp = true;
    thefitfunc = @(a2, a1, m, c, x) -a2*x.^2 - a1*x + m*log(x) + c;
    fitoptions = {'StartPoint', [1, 0, 1, 100]};
    %thefitfunc = @(a, c, x) -a*x + log(x) + c;
    %fitoptions = {'StartPoint', [1, 100]};
    display(thefitfunc);
  end    
  % --- prepare data for the fit ---
  [MaxP, MaxPInd] = max(P);
  FitDataIdx = (P > opts.FitThresFrac*MaxP);
  if (opts.XIsOneMinus)
    % reverse direction of x data points because of 1-Fid
    ignore_end_points = opts.IgnoreStartPoints;
    ignore_start_points = opts.IgnoreEndPoints;
  else
    ignore_start_points = opts.IgnoreStartPoints;
    ignore_end_points = opts.IgnoreEndPoints;
  end
  if (ignore_start_points > 0)
    FitDataIdx(1:0+ignore_start_points) = logical(0);
  end
  if (ignore_end_points > 0)
    FitDataIdx(end-(ignore_end_points):end) = logical(0);
  end
  % --- prepare fit data x, y and weights ---
  FitDataX = XX(FitDataIdx);
  if (fitlogp)
    FitDataY = LogP(FitDataIdx);
    FitDataErrors = ErrorLogP(FitDataIdx);
  else
    FitDataY = P(FitDataIdx);
    FitDataErrors = ErrorP(FitDataIdx);
  end
  %
  % see the Curve Fitting Toolbox manual (e.g. PDF): weight_i = 1/(sigma_i)^2
  % Here, our errors are standard deviations = sigma's
  %
  FitDataWeights = 1 ./ (FitDataErrors.^2);

  % --- now, do the fit ---
  [thefit, gof] = fit(FitDataX, FitDataY, thefitfunc, ...
                      'Weights', FitDataWeights, ...
                      fitoptions{:} ...
                      );

  if (fitlogp)
    evalfitlogp = @(x) thefit(x);
    evalfitp = @(x) exp(thefit(x));
  else
    evalfitlogp = @(x) log(thefit(x));
    evalfitp = @(x) thefit(x);
  end

  display(thefit);
  
  
  %
  % "deskew" the fit to get the quantum error bars.
  %
  QuErrorBars = struct;
  if (opts.QuErrorBars)
    % we know there is no custom fit.
    [a x0 y0] = deskew_logmu_curve(thefit.a2, thefit.a1, thefit.m, thefit.c);
    % the "quantum error bars" per se:
    QuErrorBars.f0 = xtof(x0);
    QuErrorBars.Delta = 1/sqrt(a);
    QuErrorBars.gamma = thefit.m / (6 * a.^2 * x0.^3);
    % the normalization constant:
    QuErrorBars.y0 = y0;
    % and the intermediary values:
    QuErrorBars.a = a;
    QuErrorBars.m = thefit.m;
    QuErrorBars.x0 = x0;
    display(QuErrorBars);
  end
  
  if (~isequal(opts.FigHandleLogP, -1))
    figure(opts.FigHandleLogP);
    if (opts.ClearFigures)
      clf;
    end
    hold on;
    set(gca, 'YScale', 'linear'); % seems to be needed to get stuff right if NoClearFigures?
    
    errorbar(plotx_trans(XX(P_IndNonZero)), P(P_IndNonZero), ErrorP(P_IndNonZero), ...
             'Marker', '.', 'LineStyle', 'none', 'Color', [0.6, 0.6, 1.0]);
    if (opts.PlotFitPoints)
      plot(plotx_trans(FitDataX), P(FitDataIdx), 'rx');
    end

    if (opts.PlotDeskewedGaussian)
      plot(plotx_trans(xxpts), exp(-QuErrorBars.a.*(xxpts-QuErrorBars.x0).^2 + ...
                                   QuErrorBars.y0), 'g-');
    end

    plot(plotx_trans(xxpts), evalfitp(xxpts), 'r-');
    
    set(gca, 'YScale', 'log');
  end
  
  % ---
  % plot the fit back onto the first figure
  if (~isequal(opts.FigHandleP, -1))
    figure(opts.FigHandleP);
    hold on;

    if (opts.PlotDeskewedGaussian)
      plot(plotx_trans(xxpts), exp(-QuErrorBars.a.*(xxpts-QuErrorBars.x0).^2 + ...
                                   QuErrorBars.y0), 'g-');
    end

    plot(plotx_trans(xxpts), evalfitp(xxpts), 'r-');
  end
  
  % ---
  % Prepare return data
  % ---
  if (nargout > 0)
    dat = struct;
    %    dat.Fidelity = Fidelity;% obsolete name
    %    dat.OneMinusFidelity = OneMinusFidelity;% obsolete name
    dat.F = F;
    dat.X = XX;
    dat.AvgCounts = AvgCounts;
    dat.ErrorAvgCounts = Error;
    dat.HistogramNormalization = histnorm;
    dat.P = P;
    dat.LogP = LogP;
    dat.ErrorP = ErrorP;
    dat.ErrorLogP = ErrorLogP;
    dat.P_IndNonZero = P_IndNonZero;
    dat.fitlogp = fitlogp;
    dat.FitDataIdx = FitDataIdx;
    dat.FitDataX = FitDataX;
    dat.FitDataY = FitDataY;
    dat.FitDataErrors = FitDataErrors;
    dat.FitDataWeights = FitDataWeights;
    dat.thefitfunc = thefitfunc; % in whatever fit space
    dat.evalfitlogp = evalfitlogp; % always log(P)
    dat.evalfitp = evalfitp; % always P
    dat.thefit = thefit;
    dat.gof = gof;
    dat.FigHandleP = opts.FigHandleP;
    dat.FigHandleLogP = opts.FigHandleLogP;
    dat.QuErrorBars = QuErrorBars;
    
    %
    % Also calculate chi-squared of the fit.
    %
    % Note the \sum_i (data_i - theoretical_i) / error2_i  ==  dat.gof.sse .
    %
    dat.gofchi2red = dat.gof.sse / (numel(dat.FitDataIdx) - numargs(fittype(thefit)) - 1);
  end

end


function figh = get_figure_handle(val, histfname, whichplot)
  if (isempty(val))
    % get default name from histogram file name. Remove "-histogram.csv" from histogram
    % fname first.
    if (numel(histfname) >= 14 && strcmp(histfname((end-13):end), '-histogram.csv'))
      histfname = histfname(1:(end-14));
    elseif (numel(histfname) >= 4 && strcmp(histfname((end-3):end), '.csv'))
      histfname = histfname(1:(end-4));
    end
    if (strcmp(whichplot, 'p'))
      figh = mynamedfigure(['figp-' histfname]);
    elseif (strcmp(whichplot, 'logp'))
      figh = mynamedfigure(['figlogp-' histfname]);
    else
      error('analyze_tomorun_histogram:get_figure_handle:badwhichplot', ...
            'internal error: unknown whichplot=%s', whichplot);
    end
  elseif (isnumeric(val))
    % including -1
    figh = val;
  elseif (ischarstring(val))
    figh = mynamedfigure(val);
  else
    error('analyze_tomorun_histogram:badFigureHandle', 'Bad figure handle: %s', ...
          dispstr(val));
  end
end
