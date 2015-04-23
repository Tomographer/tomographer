function dat = analyze_tomorun_histogram(histfname, varargin)

  optdefs = struct;
  optdefs.FitThresFrac = struct('arg', 'n', 'default', 1e-3);
  optdefs.IgnoreStartPoints = struct('arg', 'n', 'default', 0);
  optdefs.FigHandleP = struct('arg', 'any', 'default', [], ...
                              'validator', @(x) get_figure_handle(x, 'intgr-fid-epsilon'));
  optdefs.FigHandleLogP = struct('arg', 'any', 'default', [], ...
                                 'validator', @(x) get_figure_handle(x, 'fit-intgr-fid-epsilon'));
  optdefs.ClearFigures = struct('switch', true);
  optdefs.CustomFit = struct('switch', false);
  optdefs.FitModel = struct('arg', 'fh*', 'default', [], ...
                            'imply', {{'CustomFit'}});
  optdefs.FitWhich = struct('arg', 's*', 'default', [], ...
                            'validator', validator_enum('LogP', 'P'), ...
                            'imply', {{'CustomFit'}});

  [opts, ~] = parse_opts(optdefs, varargin);
  
  FIT_THRES_FRAC = opts.FitThresFrac;
  IGNORE_START_POINTS = opts.IgnoreStartPoints;

  data = importdata(histfname);
  
  Fidelity = data.data(:,1);
  AvgCounts = data.data(:,2);
  Error = data.data(:,3);
  
  OneMinusFidelity = 1 - Fidelity;
 
  % Normalize to probability density
  P = AvgCounts/trapz(-OneMinusFidelity,AvgCounts);
  ErrorP = Error/trapz(-OneMinusFidelity,AvgCounts);
  
  % get the "support" of P -- where P != 0
  P_IndNonZero = find(P>0);
  % and the width of one bin (NOTE: ASSSUMPTION THAT BINS EQUALLY SPACED)
  %bin_width = abs(Fidelity(2)-Fidelity(1))
  
  figure(opts.FigHandleP);
  if (opts.ClearFigures)
    clf;
  end
  errorbar(OneMinusFidelity, P, ErrorP, ...
           'Marker', '.', 'LineStyle', 'none');
  set(gca, 'YScale', 'linear')

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
    display(thefitfunc);
    fitoptions = {'StartPoint', [1, 1, 100]};
  end    
  % --- prepare data for the fit ---
  [MaxP, MaxPInd] = max(P);
  FitDataIdx = P>FIT_THRES_FRAC*MaxP;
  FitDataIdx = FitDataIdx(1:end-IGNORE_START_POINTS);
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

  figure(opts.FigHandleLogP);
  if (opts.ClearFigures)
    clf;
  end
  hold on;
  %plot(thefit, OneMinusFidelity, log(P));
  errorbar(OneMinusFidelity(P_IndNonZero), P(P_IndNonZero), ErrorP(P_IndNonZero), ...
           'Marker', '.', 'LineStyle', 'none', 'Color', [0.6, 0.6, 1.0]);
  plot(FitDataX, exp(FitDataY), 'rx');
  xxmin = min(OneMinusFidelity(P_IndNonZero));
  xxmax = max(OneMinusFidelity(P_IndNonZero));
  xx = linspace(xxmin, xxmax, 200);
  plot(xx, evalfitp(xx), 'r-');
  set(gca, 'YScale', 'log');
  
  % ---
  % plot the fit back onto the first figure
  figure(opts.FigHandleP);
  hold on;
  plot(xx, evalfitp(xx), 'r-');
  
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
    figh = val;
  elseif (ischarstring(val))
    figh = mynamedfigure(val);
  else
    error('analyze_tomorun_histogram:badFigureHandle', 'Bad figure handle: %s', ...
          dispstr(val));
  end
end
