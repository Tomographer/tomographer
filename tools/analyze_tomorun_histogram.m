function dat = analyze_tomorun_histogram(histfname, varargin)
%
% Analyze a histogram produced by tomorun C++ integrator
% 
% dat = analyze_tomorun_histogram(histogramfilename, ...)
%
% Run analyze_tomorun_histogram('', 'Help') for more information.
%

  optdefs = struct;
  optdefs.FitThresFrac = struct('arg', 'n', 'default', 1e-3);
  optdefs.FitThresFrac.help = ['Fraction of maximum value above which to consider points ' ...
                      'for fit. Points below this threshold will be ignored.'];
  optdefs.IgnoreStartPoints = struct('arg', 'n', 'default', 0);
  optdefs.IgnoreStartPoints.help = ['A number of points which should be ignored, starting ' ...
                      'from the f=0 side.'];
  optdefs.CenterBins = struct('switch', true);
  optdefs.CenterBins.help = ['tomorun produces Fidelity values which correspond to lower ' ...
                      'bin values. Use this option to center the bins, i.e. move the x ' ...
                      'axis so that the x label corresponds to the center of a bin, and ' ...
                      'not the lower limit.'];
  optdefs.FigHandleP = struct(...
      'arg', 'any', 'default', [], ...
      'validator', @(x) get_figure_handle(x, 'intgr-fid-epsilon'));
  optdefs.FigHandleP.help = ['Figure to use for plotting the probability density. Specify ' ...
                      'a figure handle, a name for mynamedfigure(), leave empty for ' ...
                      'default, or specify -1 to not produce figures.'];
  optdefs.FigHandleLogP = struct(...
      'arg', 'any', 'default', [], ...
      'validator', @(x) get_figure_handle(x, 'fit-intgr-fid-epsilon'));
  optdefs.FigHandleLogP.help = ['Figure to use for plotting the logarithm of the ' ...
                      'probability density. Specify a figure handle, a name for ' ...
                      'mynamedfigure(), leave empty for default, or specify -1 to not ' ...
                      'produce figures.'];
  optdefs.ClearFigures = struct('switch', true);
  optdefs.ClearFigures.help = ['If set, then a figure will be cleared of any previous ' ...
                      'content before drawing to it (the default).'];
  optdefs.CustomFit = struct('switch', false);
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

  [opts, ~] = parse_opts(optdefs, varargin);
  
  data = importdata(histfname);
  
  Fidelity = data.data(:,1);
  AvgCounts = data.data(:,2);
  Error = data.data(:,3);
  
  if (opts.CenterBins)
    Fidelity = Fidelity + (Fidelity(2)-Fidelity(1)) ./ 2.0;
  end

  OneMinusFidelity = 1 - Fidelity;

  if (any(OneMinusFidelity==0))
    warning('analyze_tomorun_histogram:zeroInOneMinusFidelity', ['Found a zero value in ' ...
                        'OneMinusFidelity. fit might complain about model computing Inf.']);
  end

  % Normalize to probability density
  P = AvgCounts/trapz(-OneMinusFidelity,AvgCounts);
  ErrorP = Error/trapz(-OneMinusFidelity,AvgCounts);
  
  % get the "support" of P -- where P != 0
  P_IndNonZero = find(P>0);
  % and the width of one bin (NOTE: ASSSUMPTION THAT BINS EQUALLY SPACED)
  %bin_width = abs(Fidelity(2)-Fidelity(1))
  
  if (~isequal(opts.FigHandleP, -1))
    figure(opts.FigHandleP);
    if (opts.ClearFigures)
      clf;
    end
    errorbar(OneMinusFidelity, P, ErrorP, ...
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
    thefitfunc = @(a, m, c, x) -a*x + m*log(x) + c;
    fitoptions = {'StartPoint', [1, 1, 100]};
    %thefitfunc = @(a, c, x) -a*x + log(x) + c;
    %fitoptions = {'StartPoint', [1, 100]};
    display(thefitfunc);
  end    
  % --- prepare data for the fit ---
  [MaxP, MaxPInd] = max(P);
  FitDataIdx = (P > opts.FitThresFrac*MaxP);
  FitDataIdx = FitDataIdx(1:end-opts.IgnoreStartPoints);
  % --- prepare fit data x, y and weights ---
  FitDataX = OneMinusFidelity(FitDataIdx);
  if (fitlogp)
    FitDataY = LogP(FitDataIdx);
    FitDataWeights = 1 ./ ErrorLogP(FitDataIdx);
  else
    FitDataY = P(FitDataIdx);
    FitDataWeights = 1 ./ ErrorP(FitDataIdx);
  end
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

  % ---
  %thefitfunc = @(a, b, m, c, x) -a*x -b*x.^2 + m*log(x) + c
  %evalfitlogp = @(p, x) thefitfunc(p.a, p.b, p.m, p.c, x);
  %[thefit, gof] = fit(FitDataX, FitDataY, thefitfunc, ...
  %                    'Weights', FitDataWeights, ...
  %                    'StartPoint', [1, 0, 1, 100] ...
  %                    );
  % ---
  %thefitfunc = @(a, c, x) -a*x + log(x) + c
  %evalfitlogp = @(p, x) thefitfunc(p.a, p.c, x);
  %[thefit, gof] = fit(FitDataX, FitDataY, thefitfunc, ...
  %                    'Weights', FitDataWeights, ...
  %                    'StartPoint', [1, 100] ...
  %                    );
  % ---
  %thefitfunc = @(a, c, x) c*x.*exp(-a*x)
  %evalfitlogp = @(p, x) log(thefitfunc(p.a, p.c, x));
  %[thefit, gof] = fit(FitDataX, P(FitDataIdx), thefitfunc, ...
  %                    'Weights', 1 ./ ErrorP(FitDataIdx), ...
  %                    'StartPoint', [1, 100] ...
  %                    );
  % ---
  
  display(thefit);

  if (~isequal(opts.FigHandleLogP, -1))
    figure(opts.FigHandleLogP);
    if (opts.ClearFigures)
      clf;
    end
    hold on;
    set(gca, 'YScale', 'linear'); % seems to be needed to get stuff right if NoClearFigures?
    %plot(thefit, OneMinusFidelity, log(P));
    errorbar(OneMinusFidelity(P_IndNonZero), P(P_IndNonZero), ErrorP(P_IndNonZero), ...
             'Marker', '.', 'LineStyle', 'none', 'Color', [0.6, 0.6, 1.0]);
    plot(FitDataX, P(FitDataIdx), 'rx');
    xxmin = min(OneMinusFidelity(P_IndNonZero));
    xxmax = max(OneMinusFidelity(P_IndNonZero));
    xx = linspace(xxmin, xxmax, 200);
    plot(xx, evalfitp(xx), 'r-');
    set(gca, 'YScale', 'log');
  end
  
  % ---
  % plot the fit back onto the first figure
  if (~isequal(opts.FigHandleP, -1))
    figure(opts.FigHandleP);
    hold on;
    plot(xx, evalfitp(xx), 'r-');
  end
  
  % ---
  % Prepare return data
  % ---
  if (nargout > 0)
    dat = struct;
    dat.Fidelity = Fidelity;
    dat.OneMinusFidelity = OneMinusFidelity;
    dat.AvgCounts = AvgCounts;
    dat.ErrorAvgCounts = Error;
    dat.P = P;
    dat.LogP = LogP;
    dat.ErrorP = ErrorP;
    dat.ErrorLogP = ErrorLogP;
    dat.P_IndNonZero = P_IndNonZero;
    dat.fitlogp = fitlogp;
    dat.FitDataIdx = FitDataIdx;
    dat.FitDataX = FitDataX;
    dat.FitDataY = FitDataY;
    dat.FitDataWeights = FitDataWeights;
    dat.thefitfunc = thefitfunc; % in whatever fit space
    dat.evalfitlogp = evalfitlogp; % always log(P)
    dat.evalfitp = evalfitp; % always P
    dat.thefit = thefit;
  end

end


function figh = get_figure_handle(val, defaultnamedfigure)
  if (isempty(val))
    figh = mynamedfigure(defaultnamedfigure);
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
