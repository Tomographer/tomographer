function analyze_tomorun_histogram(histfname, varargin)

  optdefs = struct;
  optdefs.FitThresFrac = struct('arg', 'n', 'default', 1e-3);
  optdefs.IgnoreStartPoints = struct('arg', 'n', 'default', 0);
  
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
  
  mynamedfigure('intgr-fid-epsilon');
  clf;
  errorbar(OneMinusFidelity,P,ErrorP, 'Marker', '.', 'LineStyle', 'none');
  set(gca, 'YScale', 'linear')

  % calculate logarithm of P
  LogP = log(P);
  ErrorLogP = ErrorP ./ P; % delta(log x) = delta(x) / x

  % do the fit
  [MaxP, MaxPInd] = max([0;P(2:end-1)]); % note: prepend zero so indexes match in P
  FitDataIdx = P>FIT_THRES_FRAC*MaxP;
  LastFitDataIdx = find(FitDataIdx,1);
  FitDataX = OneMinusFidelity(FitDataIdx);
  FitDataY = LogP(FitDataIdx);
  FitDataWeights = 1 ./ ErrorLogP(FitDataIdx);
  % possibly restrict which points are used
  FitDataX = FitDataX(1:end-IGNORE_START_POINTS);
  FitDataY = FitDataY(1:end-IGNORE_START_POINTS);
  FitDataWeights = FitDataWeights(1:end-IGNORE_START_POINTS);

  % --- do fit ---
  thefitfunc = @(a, b, c, x) -a*x + b*log(x) + c
  [thefit, gof] = fit(FitDataX, FitDataY, thefitfunc, ...
                      'Weights', FitDataWeights, ...
                      'StartPoint', [1, 1, 100] ...
                      );
  % ---
                    
  display(thefit);

  mynamedfigure('fit-intgr-fid-epsilon');
  clf;
  hold on;
  %plot(thefit, OneMinusFidelity, log(P));
  errorbar(OneMinusFidelity, P, ErrorP, 'Marker', '.', 'LineStyle', 'none', 'Color', [0.6, 0.6, 1.0]);
  plot(FitDataX, exp(FitDataY), 'rx');
  xx = linspace(min(FitDataX), max(FitDataX),200);
  plot(xx, exp(thefitfunc(thefit.a, thefit.b, thefit.c, xx)), 'r-');
  set(gca, 'YScale', 'log');
  

end