function [ opts args ] = parse_opts(optdefs, argin)
    
% Utility to parse options given as optional arguments to a function call
%
% Much like UNIX command lines, one often wants to alter the behavior of a
% function via the use of options, which may alter the specific behavior of
% the function:
%
%    result = myfunc(a, b, 'Epsilon', 1e-6, 'DrawPlots', ...
%                    'Color', [1.0, 0.5, 0.5])
%
% The parse_opts() function is a utility which functions may invoke to easily parse
% such option arguments.
%
%
% USAGE
% =====
%
%   [ opts, args ] = parse_opts(optdefs, argin);
%
% Input Arguments:
%
%   - optdefs   a structure containing the specification of the possible
%               options
%
%   - argin     the list of function arguments to parse. You'll most likely
%               want to pass varargin here.
%
% Output Arguments:
%
%   - opts      the parsed options formatted into a sane structure.
%
%   - args      additional arguments passed to the function.
%
%
% SPECIFYING POSSIBLE OPTIONS
% ===========================
%
%   optdefs = struct;
%   optdefs.Option1 = struct(Spec-Option-1);
%   optdefs.Option2 = struct(Spec-Option-2);
%   [...]
%
% Each field of the optdefs structure is an option name (exception:
% meta-options, see below). The value of the corresponding field is itself a
% structure, specifying the kind of option this is. Options may be of the
% following kind:
%
%     - a boolean switch;
%
%     - an option requiring a mandatory argument;
%
%     - a mode option;
%
%     - or a shortcut option (shorthand for another option specification).
%
% Each of these kind are specified with the Spec-Option structure and may have
% several settings.
%
% As a general rule, the returned opts structure contains all possible options
% specified in optdefs with their corresponding values set by the user. If a
% default value was provided in the option specification (making the option not
% mandatory), then that default value is set in the returned structure if the
% user omit the option.
%
% BEWARE: Take special care if the value of some field should be a cell
% array. The syntax
%
%     struct('field', {'field-value-1', 'field-value-2'})   % !! WRONG !!
%
% is interpreted by MATLAB as creating a cell array of structures (!). Use
% double braces to make sure that you have a single structure, with a cell array
% field value:
%
%     struct('field', {{'field-value-1', 'field-value-2'}})   % OK
%
%
% Boolean Options
% ---------------
%
% These are switches usually for enabling or disabling a specific feature, for
% example:
%
%     myfunction(bla,bla,bla, 'ColorPlot', true);
%
% Boolean options are specified as switches, with a default value:
%
%     optdefs.Option1 = struct('switch', true);  % on by default
%     optdefs.Option2 = struct('switch', false); % off by default
%
% They may be declared not to have a default value. The option becomes mandatory
% and the user has to specify a value for it.
% 
%     optdefs.Option1 = struct('switch', 'mandatory');
%
% When you specify a boolean option 'Option1', several variants are
% automatically recognized, namely: 'WithOption1', 'YesOption1', '+Option1' as
% enabling the option and 'WithoutOption1', and 'NoOption1' as disabling that
% option.
%
% If you specify an empty default value, i.e. struct('switch', []), then the
% option will actually effectively be a 3-state option: "specified true",
% "specified false" or "unspecified". The corresponding field in the opts
% structure is always set to either an empty value (option unspecified), or
% true/false if the option was specified.
%
% Boolean options accept an optional argument. If the argument immediately
% following the option is parsed as a boolean value (eg. 't', 'y', 'no', 'off',
% 1, false, ...)  then it is used to confirm or negate the option itself. This
% applies also to the variants 'WithOption1', 'NoOption1', etc. You may instruct
% the option parser not to look for boolean arguments, see the opt_bool_arg
% meta-option below.
%
% A field in the returned opts structure is created, with the name of the option
% (here 'Option1'), with value true/false. This field is always created, even if
% the option wasn't specified (in which case its default value is provided). If
% a variant such as 'NoOption1' was provided, then it is still opts.Option1 that
% is set to the appropriate value.
%
% For convenience, if you declare an option with an empty structure, then it is
% interpreted as a boolean switch with default value false. (More precisely,
% this happens for any option specification that is not declared as accepting
% any argument, that is not a shortcut nor a mode.) For example:
%
%     optdefs.Option1 = struct();  % boolean switch with default false
%
%
% Options With Arguments
% ----------------------
%
% These are used to give additional data to your procedure, eg. a file name,
% plot title etc.
%
%     myfunction(bla,bla, 'PlotTitle', 'Title of Plot', 'PlotColor', [1 0 0])
%
% You can specify such options by providing the 'arg' key, and specifying
% which type you expect as argument:
%
%     optdefs.Option1 = struct('arg', 's', [...])  % string argument
%     optdefs.Option1 = struct('arg', 'n', [...])  % numerical argument
%     optdefs.Option1 = struct('arg', 'any', [...])  % any argument
%   
% The string set to the 'arg' property resticts the possible argument type that
% can be given by the user:
%
%     'any': no restriction is enforced, any argument can be given;
%     's': enforce a (single) string argument (ischarstring());
%     'n': enforce a numeric argument (isnumeric());
%     'N': enforce a single number (isnumeric() and numel()==1);
%     'I': enforce a single integer (isnumeric() and numel()==1 and x==floor(x));
%     'cell': argument has to be a cell array (iscell());
%     'fh': argument has to be a function handle (isa(x, 'function_handle'));
%     'b': this is a boolean option, and it will be treated like a switch
%          (with the full boolean option processing machinery, with possible
%          prefixes etc.). You may specify a default value, too, with
%          'default'.
%
% For any more specific checks and enforcements, use 'argcheck' or 'validator'.
%
% Default values may be provided with the 'default' field, making the option not
% mandatory:
%
%     optdefs.Option1 = struct('arg', 's', 'default', 'Title1', [...])
%
% The value provided by the user is saved in the returned opts structure under
% the field name corresponding to the option name (here 'Option1'). If the
% option has a default value, and the option is not specified, the field is
% assigned the default value. The given default value must also satisfy
% requirements set by the argument's type (e.g. 'n'), by any given 'argcheck'
% functions or a 'validator' function.
%
% If the 'emptyarg' field is given and set to 1 or true, then if an argument is
% an empty value (i.e., []), then the argument is accepted regardless of any
% given argchecks or validators. This can be useful if you want to allow the
% user to either specify a valid value or an empty value. You may also add a '*'
% to the type in the 'arg' to specify this, for example:
%
%     optdefs.Option1 = struct('arg', 'n*')
%     optdefs.Option1 = struct('arg', 'n', 'emptyarg', true)  % same
%
% You may specify some conditions that the argument must fulfill, more
% specifically than with the type specified by the 'arg' field. There are two
% possibilities: you may either check that the argument satisfies some
% properties (use 'argcheck'), or you may try to fix the argument to "normalize"
% it (use 'validator').
%
% If you pass a function handle to the 'argcheck' field, then whenever this
% option is given with an argument, this argcheck function is called. The
% function should take a single input, the argument, and should output a value
% that evaluates logically to true if the value is acceptable or false if the
% value is not acceptable. The function may also raise an exception if it
% prefers. It is possible to specify more than one argcheck function by passing
% a cell array of functions, in which case the argument is considered valid only
% if all functions accept the argument.
%   
% If you pass a function handle to the 'validator' field, then whenever this
% option is given with an argument, the validator function is called. The
% function should take a single input, the argument, and should have a single
% output, the normalized argument. The output of this function is used as the
% argument value instead of the argument that was actually given by the user
% (i.e., the function allows to replace an argument by some "normalized"
% version). If the validator function finds an input argument unacceptable, it
% may issue an exception with error().
%
% Argument types (e.g. 'n', 's', etc.) behave exactly as if the corresponding
% constraints were given as an 'argcheck' function. All 'argcheck' functions
% are tested (if any) before calling the 'validator' function (if any).
%
% See also: validator_num() for some very useful validators for numeric inputs,
% such as enforcing positive semidefinite matrices.
%
% You may specify as many 'argcheck' functions as you want, but only one
% 'validator'.
%
%
% Mode Options
% ------------
%
% Mode options are meant to specify a specific behavior among several possible
% functioning modes, for example a method to use to solve a problem. Mode
% options may also be used to specify some sort of "command", behaving like a
% simplified boolean option.
%
% There are two type of mode options:
%
%   - exclusive mode options indicate a choice among several possibilities;
%
%   - otherwise the mode option is not linked to any other option and just
%     corresponds to, e.g., a specific command.
%
% --> Exclusive Mode Options <--
%
%   The following example illustrates the use of exclusive mode options. The
%   different choices (here 'Hradil', 'Fmincon' and 'CVX') are declared as
%   options, with their specification declaring that they belong to mode choice
%   'Method'.
%
%       optdefs.Hradil = struct('mode', 'Method');
%       optdefs.Fmincon = struct('mode', 'Method');
%       optdefs.CVX = struct('mode', 'Method');
%
%   The user may then specify either the mode directly, or as argument to the
%   option 'Method'.
%
%       rhoMLE = find_mle(data, 'Hradil'); % use Hradil method
%       rhoMLE = find_mle(data, 'Method', 'Hradil'); % same
%
%   The returned opts structure contains one field for each mode, set to true or false
%   depending on whether that mode is on, and additionally has a field with the mode
%   name (here 'Method') set to the corresponding choice, e.g.
%
%       display(sprintf('Using mode %s.', opts.Method));
%       if (opts.Hradil)
%         % use Hradil method
%       elseif (opts.CVX)
%         % ...
%       [...]
%
%   You may specify a default mode, making the specification of a mode not mandatory for
%   the user,
%
%       optdefs.Hradil = struct('mode', 'Method', 'isdefault', true);
%
% --> Non-Exclusive Mode Options <--
%
%   These may be specified by setting the 'mode' field to a non-string value in the
%   option specification (the actual value is irrelevant),
%
%       optdefs.ShowPlots = struct('mode', 1);
%
%   Again, the corresponding field in opts is always set. It is set to true if the
%   option was specified, and to false otherwise.
%
%   Such options are essentially boolean options (specified or not specified), but
%   they do not have the boolean option features provided by switches, for example,
%   the option 'NoShowPlots' or 'WithoutShowPlots' will NOT be recognized.
%
%
% Option Dependencies, Implications
% ---------------------------------
%
% It is possible to define some option dependencies by means of
% "implications". If any option also specifies an 'imply' field, then whenever
% that option is seen, then the options specified in the 'imply' field will be
% parsed, as if they had also been given by the user. Beware that this may lead
% to conflicts, which are reported as errors.
%
% The 'imply' field for an option specification must be given in form of a cell
% array of options and possible values, specified as they would have been
% specified by the user as arguments. For example,
%
%     optdefs.DoDrawPlots = struct('mode', 'WhatToDo');
%     optdefs.DoSmokeACigar = struct('mode', 'WhatToDo');
%     optdefs.DoPlayViolin = struct('mode', 'WhatToDo');
%     optdefs.PlotsHaveNames = struct('switch', true);
%     optdefs.PlotName = struct('arg', 's', ...
%                               'imply', {{'DoDrawPlots', ...
%                                          'PlotsHaveNames', true}});
%
% [Beware MATLAB's catch: if you give a cell array in the struct() funtion,
% you'll get unexpected results because struct() will construct a cell array of
% structures! Use double-braces for correct results.]
%
% You may also specify an 'imply_if' field. This causes the implications to be
% parsed only if the argument to the option satisfies a condition. The value to
% 'imply_if' should be a function handle taking as argument the value of the
% option. For example, for a bool switch:
%
%     optdefs.InitializeDebugLog = struct('mode', 1);
%     optdefs.ProduceDebugMessages = struct('switch', false, ...
%                            'imply', {{'InitializeDebugLog'}},
%                            'imply_if', @(v) v);
%
% By default, 'imply_if' is an empty array `[]', meaning that the
% implications are always parsed whenever this option is seen.
%
% WARNING: implications are only taken into account if the option is seen in the
% input. In particular, implications don't kick in for a default option value!
%
%
%
% Shortcut Options
% ----------------
%
% Shortcut options are a way of specifying a specific option sequence with a
% concise syntax. For example, if you declare
%
%     optdefs.PerformConsistencyChecks = struct('switch', false);
%     optdefs.ProduceLogs = struct('switch', false);
%     optdefs.LogFileName = struct('arg', 's');
%     optdefs.Debug = struct('shortcut', {{'PerformConsistencyChecks', ...
%                                          'ProduceLogs', ...
%                                          'LogFileName', 'debug-log.txt'}});
%
% Then if the user specfies the option 'Debug', the option parser behaves as if
% the sequence of options 'PerformConsistencyChecks', ... had been given
% instead.
%
% Shortcut options may not take arguments. It is an error to specify 'arg' or
% 'switch' to a shortcut option.
%
% A shortcut option may be empty. Also, a shortcut option may have
% implications. Specifying an implication for a shortcut behaves slightly
% differently, for example in which error is reported in case of option
% conflicts (repeated option, versus option conflict).
%
% For convenience, you may declare a shortcut which sets an option <optname> to
% a given value (<optname> must have been previously declared in optdefs), by
% using the syntax:
%   
%     optdefs.Option1 = struct('set', <optname>, 'to', <given-value>)
%   
% The mechanism of the 'set'/'to' option is the same as an empty shortcut with
% an 'imply' field.
%
%
% Repeatable Options
% ------------------
%
% By default, it is an error to repeat an option a second time. This is for
% consistency and for eased debugging (to avoid e.g. specifying the option twice
% by mistake, and then being confused by unexpected behavior). However, it may
% make sense for some options to repeat them, in which case the given values are
% cumulated in a cell array. You may set the field 'repeat' to true to indicate
% this:
%   
%     optdefs.Option1 = struct('arg', 'n', 'repeat', true);
%   
% Then, if the user repeats the option, the values are cumulated into a cell
% array. For example:
%   
%     myfunction('Option1', 1, 'Option1', 2);
%   
% [In fact, if an option is not declared as repeatable, then the parser will
% still accept if you repeat the option but only if you specify the same value
% each time. (This is such that if an option is implied by a shortcut or an
% implication, then you do not have to worry not to specify it again even if you
% have the same value.)]
%
% If an option is repeatable, it will also no longer conflict if a value is
% also implied by another option. In this case, the values are cumulated.
%
%
% Documenting Options (Help)
% --------------------------
%
% Each option may be given an additional field called 'help'. You should specify
% in this field a description of what the option does, and for example possible
% values and how they are interpreted. This help text is used to format a
% help message whenever the user provides the 'Help' option. See the opt_help
% metaoption below if you don't want the parser to display a help text.
%
%
% OPTION PARSING SETTINGS (META-OPTIONS)
% ======================================
%
% It is possible to alter the behavior of the option parsing, by setting a few
% "meta-options". Any field beginning with 'opt_' in optdefs is treated as a
% special flag to the parser and will not be treated as an option name. Are
% understood in particular:
%
%   * optdefs.opt_case_sensitive  =  true/false  [default: true]
%
%     Perform case-sensitive option name matching if true. If false, option
%     names may be spelled in any CaSE and will be recognized.
%
%   * optdefs.opt_allow_args  =  true/false  [default: false]
%
%     If set to true, then any given arguments that cannot be parsed are
%     returned into the args list. If false, then any unparsed argument is an
%     error (the default).
%
%   * optdefs.opt_args_check = [min, max]   [default: [0, Inf]]
%     optdefs.opt_args_check = fixed_number
%
%     If opt_allow_args is true, then we will check that no less than min, and
%     no more than max, positional args are provided to the function call. If a
%     fixed number is given then exactly that number of arguments is expected.
%
%   * optdefs.opt_bool_arg  =  true/false  [default: true]
%
%     If set to true, then boolean options accept an optional boolean argument:
%     if the next argument is parsed as a boolean (eg. 'y', '1', 1, 0, 'yes',
%     'on', true, 'n', 'off', false, ...), then it confirms or negates the
%     switch. (If the next argument doesn't look like a boolean, it is parsed
%     like another regular argument.)
%   
%   * optdefs.opt_exclusive_mode_error  =  true/false  [default: true]
%
%     If set to true, then specifying two different values for an exclusive mode
%     is an error. If this is false, then the last value specified is used, and
%     a warning is issued.
%
%   * optdefs.opt_help  =  true/false  [default: true]
%
%     If set to true, then the option 'Help' will be parsed, and if
%     specified by the user, a help message will be displayed explaining how
%     to use the different options.
%
%
% EXAMPLE
% =======
%
% function parse_opts_tester(a, b, varargin)
%
%   % Option specification structure
%   optdefs = struct;
%   
%   % option parser settings (meta-options)
%   % -------------------------------------
%   optdefs.opt_allow_args = true;
%   optdefs.opt_case_sensitive = false;
%   optdefs.opt_bool_arg = true;
%   
%   % specification of options
%   % ------------------------
%   % some random options... a single mode option, two options with arguments
%   % and a shortcut.
%   optdefs.PlotStuff = struct('mode', 1);
%   optdefs.PlotName = struct('arg', 's', 'default', 'plot');
%   optdefs.Command = struct('arg', 's*', 'default', []);
%   optdefs.Create = struct('set', 'Command', 'to', 'create');
%   % some hypothetical explosion mode: implode or explode :)
%   optdefs.Explode = struct('mode', 'ExplosionMode');
%   optdefs.Implode = struct('mode', 'ExplosionMode');
%   % be verbose? boolean switch and some shortcuts
%   optdefs.Verbose = struct('switch', 1);
%   optdefs.Quiet = struct('set', 'Verbose', 'to', 0);
%   optdefs.ForceQuiet = struct('shortcut', 'NoVerbose');
%   % some hypothetical color mode: RGB, CMYK or BlackWhite
%   optdefs.RGB = struct('mode', 'ColorMode', 'isdefault', 1);
%   optdefs.CMYK = struct('mode', 'ColorMode');
%   optdefs.BlackWhite = struct('mode', 'ColorMode');
%   % another boolean swtich, default false
%   optdefs.SanityChecks = struct();
%   % another numerical option with default value, with argument normalization.
%   optdefs.PlotColor = struct('arg', 'n', 'default', [1 0 0], ...
%     'validator', @normalizecolor);
%
%   [opts args] = parse_opts(optdefs, varargin);
%
%   % These initial arguments are invisible to the option parse and can be
%   % used as usual
%   display(a);
%   display(b);
%
%   % These are the parsed options and remaining arguments
%   display(opts);
%   display(args);
%
%   % use the resulting option structure, for example:
%   if (opts.Verbose)
%     disp('Hi there!');
%   end
%
%   ...
%
% end
%
% function x = normalizecolor(value)
%   assert(isnumeric(value(:)) && isequal(size(value(:)), [3 1]), ...
%     'Color specification must be a 3-item numeric array: `%s''', num2str(value));
%   assert(min(value(:)) >= 0 && max(value(:)) <= 1, ...
%     'Color values must be between 0 and 1.');
%   x = value(:);
% end
%
  
  ;% fail if the wrong number of arguments was given.
  error(nargchk(2, 2, nargin, 'struct'));
  
  % go through the option definitions, perform sanity checks, and normalize the values
  % for our use
  optdefs = parse_opt_defs(optdefs);

  % cache the option names
  optnames = fieldnames(optdefs);
  
  % PARSE THE INPUT ARGUMENTS into seen options. doesn't do any checking or stuff
  % like that yet, but parses shortcuts and implications.
  [ seenopts args exception ] = parse_seen_options(optdefs, argin);
  if (isa(exception, 'MException'))
    throwAsCaller(exception);
  end
  
  
  % At this point, see if 'Help' option/command was provided
  if (optdefs.opt_help)
    if (isfield(seenopts, 'Help'))
      [ST I] = dbstack(1, '-completenames');
      format_opts_help(ST(1), optdefs);
      exc = MException('parse_opts:display_help_message', 'Help Message Requested [normal interrupt].');
      throwAsCaller(exc);
    end
  end
  

  % now, go through the seen arguments, parse their values, make sure that mandatory
  % options are given, etc.
  
  opts = struct;
  
  for j=1:numel(optnames)
    opt = optnames{j};
    optdef = optdefs.(opt);
    
    if (numel(opt) > 4 && strcmp(opt(1:4), 'opt_'))
      continue;
    end
    
    if (optdef.is_shortcut)
      continue;
    end

    % Option is a switch ?
    if (optdef.is_boolean)
      
      if (~isfield(seenopts, opt))
        if (~optdef.has_default)
          exception = MException('parse_opts:mandatory_bool_option', ...
            'You have not specified the mandatory switch `%s''.', opt);
          throwAsCaller(exception);
        else
          if (isempty(optdef.default))
            seenopts.(opt) = [];
          else
            seenopts.(opt) = getbool(optdef.default);
          end
        end
      end
      
      opts.(opt) = logical(seenopts.(opt));
      
      continue;
    end
    
    % Option is a mode option ?
    if (optdef.is_mode)
      
      if (optdef.is_exclusive_mode)
        % is exclusive mode
        if (~isfield(seenopts, optdef.mode))
          if (~optdefs.opt__modes.(optdef.mode).has_default_mode)
            exception = MException('parse_opts:mandatory_mode_option', ...
              'You have not specified the mandatory mode state for `%s''.', optdef.mode);
            throwAsCaller(exception);
          else
            seenopts.(optdef.mode) = optdefs.opt__modes.(optdef.mode).default_mode;
          end
        end
        opts.(opt) = logical(strcmp(seenopts.(optdef.mode), opt));
        if (opts.(opt))
          opts.(optdef.mode) = opt;
        end
      else
        % is single mode switch
        if (isfield(seenopts, opt))
          opts.(opt) = true;
        else
          opts.(opt) = false;
        end
      end
      
      continue;
    end
    
    % Option has an argument ?
    if (optdef.has_arg)
      
      if (~isfield(seenopts, opt) || numel(seenopts.(opt)) < 1)
        if (~optdef.has_default)
          exception = MException('parse_opts:mandatory_option', ...
            'You have not specified the mandatory option `%s''.', opt);
          throwAsCaller(exception);
        else
          seenopts.(opt) = { optdef.default };
        end
      end
      optargs = seenopts.(opt);
      if (~optdef.ok_repeat)
        % tolerate, if all values are equal.
        if (are_all_equal(optargs))
          % all values are equal
          optargs = optargs(1);
          seenopts.(opt) = optargs;
        end
        if (numel(optargs) > 1)
          exception = MException('parse_opts:option_repeated', ...
            'You may not repeat option `%s''.', opt);
          throwAsCaller(exception);
        end
      end

      
      bypass_argcheck_validators = false;
      
      if (optdef.allow_empty_arg)
        bypass_argcheck_validators = true;
        % check if all imputs are empty
        for jk=1:numel(optargs)
          if ~isempty(optargs{jk})
            bypass_argcheck_validators = false;
            break;
          end
        end
      end
        
      %disp(['option ' opt ':']);
      %disp(optdef);
      %display(bypass_argcheck_validators);

      if (optdef.has_argcheck && ~bypass_argcheck_validators)
        %disp(['doing argchecks for option ' opt]);
        for chkind=1:numel(optdef.argcheck)
          fncheck = optdef.argcheck{chkind};
          for jk=1:numel(optargs)
            exception = do_call_argcheck(fncheck, optargs{jk}, opt);
            if (~isempty(exception))
              throwAsCaller(exception);
            end
          end
        end
      end
      
      if (optdef.has_validator && ~bypass_argcheck_validators)
        % normalize the arguments
        fnorm = optdef.validator;
        for jk=1:numel(optargs)
          optargs{jk} = fnorm(optargs{jk});
        end
      end
      
      % ok, option is all good, store it!
      if (numel(optargs) == 1)
        opts.(opt) = optargs{1};
      else
        opts.(opt) = optargs;
      end
      
      continue;
    end
    
    % BUG!! we should not arrive here
    error('parse_opts:bug_unknown_opt_spec', ...
      'Option %s is neither bool nor with arg???!?', opt);
    
  end
  
  % if positional arguments are accepted, make sure there are the right number of them
  if (optdefs.opt_allow_args)
    if (numel(args) < optdefs.opt_args_check(1) || ...
        numel(args) > optdefs.opt_args_check(2))
      if (optdefs.opt_args_check(1) == optdefs.opt_args_check(2))
        argexpect = sprintf('%d', optdefs.opt_args_check(1));
      else
        argexpect = sprintf('between %d and %d', optdefs.opt_args_check(1), ...
                            optdefs.opt_args_check(2));
      end
      exc = MException('parse_opts:bad_args_count', ...
                       'Bad number of arguments: expected %s argument(s)', ...
                       argexpect);
      throwAsCaller(exc);
    end
  end
  
  % allow access to full, completed parsed structure optdefs through opts.opt_optdefs
  opts.opt_optdefs = optdefs;

  % opts & args were set, return.
  return;
  
end

function optdefs = parse_opt_defs(o)
  
  optdefs = o;
  if (~isfield(optdefs, 'opt_allow_args'))
    if (isfield(optdefs, 'opt_args_check') && numel(optdefs.opt_args_check) && ...
        optdefs.opt_args_check(1) > 0)
      optdefs.opt_allow_args = 1;
    else
      optdefs.opt_allow_args = 0;
    end
  end
  if (~isfield(optdefs, 'opt_args_check'))
    optdefs.opt_args_check = [0, Inf];
  end
  if (~isfield(optdefs, 'opt_case_sensitive'))
    optdefs.opt_case_sensitive = 1;
  end
  if (~isfield(optdefs, 'opt_bool_arg'))
    optdefs.opt_bool_arg = 1;
  end
  if (~isfield(optdefs, 'opt_exclusive_mode_error'))
    optdefs.opt_exclusive_mode_error = 1;
  end
  if (~isfield(optdefs, 'opt_help'))
    optdefs.opt_help = 1;
  end
  
  optdefs.opt_allow_args = getbool(optdefs.opt_allow_args);
  if (numel(optdefs.opt_args_check) == 1 && optdefs.opt_args_check >= 0)
    optdefs.opt_args_check = [ optdefs.opt_args_check, optdefs.opt_args_check ];
  end
  assert(numel(optdefs.opt_args_check) == 2 && optdefs.opt_args_check(1) <= ...
         optdefs.opt_args_check(2), 'parse_opts:optdefs:OptArgsCheckInvalid', ...
         ['`optdefs.opt_args_check'' must be a two-element array [min, max], with min <= ' ...
          'max.']);
  optdefs.opt_args_check = [ optdefs.opt_args_check(1) , optdefs.opt_args_check(2) ];
  optdefs.opt_case_sensitive = getbool(optdefs.opt_case_sensitive);
  optdefs.opt_bool_arg = getbool(optdefs.opt_bool_arg);
  optdefs.opt_exclusive_mode_error = getbool(optdefs.opt_exclusive_mode_error);
  optdefs.opt_help = getbool(optdefs.opt_help);
  
  optdefs.opt__modes = struct;

  
  % add 'Help' option, if we should add it automatically
  if (optdefs.opt_help)
    assert(~isfield(optdefs, 'Help'), 'parse_opts:optdefs:HelpOptionSpecWithAutoHelp', ...
           ['You have declared a ''Help'' option manually, but auto help is on because ' ...
            'of optdefs.opt_help. If you want your own ''Help'' option, please set ' ...
            'optdefs.opt_help to false.']);
    optdefs.Help = struct('mode', 1); % simple mode option.
    optdefs.Help.help = 'Print this help message and raise an exception.';
  end
  

  % normalize some defs and cache some values
  optnlist = fieldnames(optdefs);
  for jj=1:numel(optnlist)
    on = optnlist{jj};
    if (numel(on) > 4 && strcmp(on(1:4), 'opt_'))
      continue;
    end
    
    assert(numel(optdefs.(on)) == 1, 'parse_opts:optdefs:isactuallyarray', ...
      ['You specified an array for the option definitions. Remember that ' ...
       'struct(''key'', { values }) generates a cell array of structures, use ' ...
       ' struct(''key'', {{ values }})  or  optdefs.Option.key = { values } ' ...
       ' instead!' ]);
    
    % user-specified option
    
    optdefs.(on).has_arg = isfield(optdefs.(on), 'arg');
    optdefs.(on).has_default = isfield(optdefs.(on), 'default');
    
    assert(~ (~optdefs.(on).has_arg && optdefs.(on).has_default), ...
      'parse_opts:optdefs:defaultWithoutArg', ...
      'Default value specified to `%s'', which doesn''t have an ''arg'' field.', ...
      on);
    
    % has an implication?
    optdefs.(on).has_imply = isfield(optdefs.(on), 'imply');
    if (optdefs.(on).has_imply)
      if (~iscell(optdefs.(on).imply))
        optdefs.(on).imply = { optdefs.(on).imply };
      end
      if (~isfield(optdefs.(on), 'imply_if'))
        optdefs.(on).imply_if = [];
      end
    end
    
    % is shortcut?
    if (isfield(optdefs.(on), 'shortcut'))
      assert(~optdefs.(on).has_arg, 'parse_opts:optdefs:shortcutAndArg', ...
        'Shortcut option `%s'' may not be declared to accept an argument.', on);
      optdefs.(on).is_shortcut = true;
      if (~iscell(optdefs.(on).shortcut))
        optdefs.(on).shortcut = { optdefs.(on).shortcut };
      else
        % all ok already
      end
      assert(~optdefs.(on).has_arg, 'parse_opts:optdefs:shortcutwitharg', ...
        'Shortcut options (here `%s'') may not have arguments', on);
      assert(~isfield(optdefs.(on), 'set'), 'parse_opts:optdefs:shortcutwithsetto', ...
             'Explicit ''shortcut'' options may not have set/to clauses: `%s''.', on);
    elseif (isfield(optdefs.(on), 'set'))
      optdefs.(on).is_shortcut = true;
      assert(isfield(optdefs.(on), 'to'), 'parse_opts:optdefs:setButNoTo', ...
        'You need a ''to'' field for option `%s''', on);
      imply = { optdefs.(on).set, optdefs.(on).to };
      % note that this is an empty shortcut; the option is set via implication.
      optdefs.(on).shortcut = {};
      if (~optdefs.(on).has_imply)
        optdefs.(on).has_imply = true;
        optdefs.(on).imply = imply;
      else
        optdefs.(on).imply = [ optdefs.(on).imply ; imply ];
      end
      assert(~optdefs.(on).has_arg, 'parse_opts:optdefs:shortcutwitharg', ...
        'Shortcut options (here `%s'') may not have arguments', on);
    else
      optdefs.(on).is_shortcut = false;
    end
    
    % is a mode option?
    optdefs.(on).is_mode = isfield(optdefs.(on), 'mode');
    if (optdefs.(on).is_mode)
      assert(~optdefs.(on).has_arg, 'parse_opts:optdefs:modeAndArg', ...
        'Mode option %s may not be declared to accept an argument.', on);
      assert(~optdefs.(on).is_shortcut, 'parse_opts:optdefs:modeAndShortcut', ...
        'Option %s may not be both a mode and a shortcut', on);
      %
      optdefs.(on).is_exclusive_mode = ischarstring(optdefs.(on).mode);
      if (optdefs.(on).is_exclusive_mode)
        mod = optdefs.(on).mode;
        if (~isfield(optdefs.opt__modes, mod))
          optdefs.opt__modes.(mod) = struct;
          optdefs.opt__modes.(mod).has_default_mode = false;
          optdefs.opt__modes.(mod).list = {};
        end
        optdefs.opt__modes.(mod).list = [ optdefs.opt__modes.(mod).list, {on} ];
        if (isfield(optdefs.(on), 'isdefault'))
          if (getbool(optdefs.(on).isdefault))
            assert(~optdefs.opt__modes.(mod).has_default_mode, ...
              'parse_opts:optdefs:multipleDefaultModes', ...
              'Multiple default modes specified for modeset `%s''', mod);
            optdefs.opt__modes.(mod).has_default_mode = true;
            optdefs.opt__modes.(mod).default_mode = on;
          end
        end
      end
    end
    
    % is a boolean option?
    if (isfield(optdefs.(on), 'switch'))
      assert(~optdefs.(on).is_shortcut, 'parse_opts:optdefs:switchAndShortcut', ...
        'Option %s may not be both a switch and a shortcut', on);
       assert(~optdefs.(on).is_mode, 'parse_opts:optdefs:switchAndMode', ...
        'Option %s may not be both a switch and a mode', on);
      % shorthand for specifying boolean options. This sets 'arg' to 'b' and possibly
      % 'default' to the corresponding value.
      assert(~optdefs.(on).has_arg, 'parse_opts:optdefs:switchAndArg', ...
        'You may not specify both fields ''switch'' and ''arg'' (for opt. %s).', on);
      assert(~optdefs.(on).has_default, 'parse_opts:optdefs:switchAndHasDefault', ...
        'You may not specify both fields ''switch'' and ''default'' (for opt. %s)', on);
      optdefs.(on).arg = 'b';
      optdefs.(on).has_arg = true;
      if (ischarstring(optdefs.(on).switch) && strcmpi(optdefs.(on).switch, 'mandatory'))
        % option is mandatory
        optdefs.(on).has_default = false;
      else
        optdefs.(on).has_default = true;
        if (isempty(optdefs.(on).switch))
          optdefs.(on).default = [];
        else
          optdefs.(on).default = getbool(optdefs.(on).switch);
        end
      end
    end
    if (optdefs.(on).has_arg && strcmp(optdefs.(on).arg, 'b'))
      optdefs.(on).is_boolean = true;
    else
      optdefs.(on).is_boolean = false;
    end

    if (~optdefs.(on).is_boolean && ...
        ~optdefs.(on).has_arg && ~optdefs.(on).is_shortcut && ~optdefs.(on).is_mode)
      % by default, it's a boolean option with default value false ... for shorthand in
      % specification...
      optdefs.(on).is_boolean = true;
      optdefs.(on).arg = 'b';
      optdefs.(on).has_arg = true;
      optdefs.(on).default = false;
      optdefs.(on).has_default = true;
    end
    
    if (optdefs.(on).has_arg && ~isempty(find(optdefs.(on).arg=='*')))
      % '*' in arg: shorthand to allow empty arg
      optdefs.(on).emptyarg = true;
      % keep those chars that are not a star:
      optdefs.(on).arg = optdefs.(on).arg(find(optdefs.(on).arg~='*'));
      assert(~isequal(optdefs.(on).arg,'b'), 'parse_opts:optdefs:emptyArgForBoolean', ...
             'Boolean options may not be declared with emptyarg (e.g. as ''b*''): %s', on);
    end
    if (isfield(optdefs.(on), 'emptyarg'))
      % whether it is OK to have an empty argument, bypassing argcheck and validators.
      assert(optdefs.(on).has_arg, 'parse_opts:optdefs:emptyargWithoutArg', ...
             ['You have specified an `emptyarg'' clause for an option without ' ...
              'argument: %s'], on);
      assert(~optdefs.(on).is_boolean, 'parse_opts:optdefs:emptyArgForBoolean', ...
             'You may not specify `emptyarg'' for boolean options: %s', on);
      optdefs.(on).allow_empty_arg = getbool(optdefs.(on).emptyarg);
    else
      optdefs.(on).allow_empty_arg = false;
    end
    
    optdefs.(on).has_argcheck = isfield(optdefs.(on), 'argcheck');
    if (optdefs.(on).has_argcheck)
      assert(optdefs.(on).has_arg, 'parse_opts:optdefs:argcheckWithoutArg', ...
        'May not specify an argcheck function for an option without argument.');
      if (~iscell(optdefs.(on).argcheck))
        optdefs.(on).argcheck = { optdefs.(on).argcheck };
      end
    end
    if (isfield(optdefs.(on), 'argnorm'))
      warning('parse_opts:optdefs:argnormobsolete', ...
        'argnorm is obsolete terminology. Use ''validator'' instead.');
      if (~isfield(optdefs.(on), 'validator'))
        optdefs.(on).validator = optdefs.(on).argnorm;
      end
    end
    optdefs.(on).has_validator = isfield(optdefs.(on), 'validator');
    if (optdefs.(on).has_validator)
      assert(optdefs.(on).has_arg, 'parse_opts:optdefs:validatorWithoutArg', ...
        'May not specify an validator function for an option without argument.');
    end

    if (optdefs.(on).has_arg && ~strcmp(optdefs.(on).arg, 'any') && ...
        ~optdefs.(on).is_boolean)
      % if we specified a specific argument type, add a corresponding argcheck call.
      thisargcheck = @(x) do_argcheck_argtype(x, optdefs.(on).arg, on);
      % and store this argument checker function to the argcheck list.
      if (~isempty(thisargcheck))
        if (~optdefs.(on).has_argcheck)
          optdefs.(on).argcheck = { thisargcheck };
        else
          optdefs.(on).argcheck = [ optdefs.(on).argcheck , { thisargcheck } ];
        end
      end
    end
    % just in case we fiddled with argchecks, update whether this option has an
    % argcheck or not.
    optdefs.(on).has_argcheck = isfield(optdefs.(on), 'argcheck');

    optdefs.(on).ok_repeat = ( isfield(optdefs.(on), 'repeat') && ...
                               getbool(optdefs.(on).repeat) );
    
  end
  
end



function [ seenopts args exception ] = parse_seen_options(optdefs, argin)
  
  exception = {};
  seenopts = struct;
  args = {};
  
  i = 1;
  while i <= numel(argin)
    
    arg = argin{i};
    
    % parse option.
    
    shortcut_tf = false;
    mode_tf = false;
    bool_tf = false;
    recopt_tf = false;
    if (ischarstring(arg))
      [shortcut_tf shortcut] = is_shortcut_option(optdefs, arg);
      % Test for a "mode" option
      [mode_tf mode_optname mode_val mode_needsarg] = is_mode_option(optdefs, arg);
      % A "bool" option is a switch that is optionally followed by boolean argument
      [bool_tf bool_optname bool_val] = is_boolean_option(optdefs, arg);
      [recopt_tf recopt_optname] = is_recognized_option(optdefs, arg);
    end
    
    % see what this argument is
    if (shortcut_tf)
      
      % this doesn't work:
      % argin([i]) = shortcut;
      
      argin = [ argin(1:(i-1)) shortcut argin((i+1):end) ];
      
      [seenopts args exception] = parse_implications(optdefs, mode_optname, seenopts, ...
                                                     args, []);
      
      % IMMEDIATLY process the shortcut on the next loop.
      continue;

    elseif (mode_tf)
      
      if (mode_needsarg)
        % get next argument
        i = i + 1;
        if (i > numel(argin))
          exception = MException('parse_opts:option_missing_mode_arg', ...
            'Missing argument to mode option `%s''.', arg);
          return;
        end
        modearg = argin{i};
        if (~ischarstring(modearg))
          exception = MException('parse_opts:option_invalid_mode_arg', ...
            'Invalid argument for mode `%s''.', arg);
          return;
        end
        mode_val = mode_optname;
        [islistitemtf mode_optname] = ...
          is_list_item(optdefs, optdefs.opt__modes.(mode_optname).list, modearg);
        if (~islistitemtf)
          exception = MException('parse_opts:option_invalid_mode_specified', ...
            'Invalid mode `%s'' specified for `%s''.', modearg, mode_optname);
          return;
        end
      end

      if (optdefs.(mode_optname).is_exclusive_mode)
        % this is exclusive mode under common modeset mode_val
        mode_key = mode_val;
        mode_kval = mode_optname;
      else
        % this is non-exclusive mode, just set that mode.
        mode_key = mode_optname;
        mode_kval = true;
      end
        % This mode was set already maybe
      if (isfield(seenopts, mode_key))
        if (optdefs.opt_exclusive_mode_error)
          % this is an error
          exception = MException('parse_opts:repeat_mode_opt_error', ...
            'Conflicting modes specified for `%s'': `%s''.', mode_key, arg);
          return;
        else
          % this is ok, just emit a warning
          warning('parse_opts:repeat_mode_opt', ...
            'Option `%s'' will override previous specification(s).', arg);
        end
      end
      
      seenopts.(mode_key) = mode_kval;
      
      [seenopts args exception] = parse_implications(optdefs, mode_optname, seenopts, ...
                                                     args, mode_kval);
      
    elseif (bool_tf)
      % this is a boolean option.
      
      % are set: bool_optname bool_val
      
      % peek to next option, see if it is boolean
      if (i < numel(argin) && optdefs.opt_bool_arg)
        [boolargok boolargtf] = looks_like_boolean(optdefs, argin{i+1});
        if (boolargok)
          % if set to zero, boolargtf inverts the value of bool_val.
          bool_val = xor(bool_val, ~boolargtf);
          i = i + 1;
        end
      end
      
      if (isfield(seenopts, bool_optname))
        warning('parse_opts:repeat_boolean_opt', ...
          'Option `%s'' will override previous specification(s).', arg);
      end
      
      seenopts.(bool_optname) = bool_val;
      
      [seenopts args exception] = parse_implications(optdefs, bool_optname, seenopts, ...
                                                     args, bool_val);
      
    elseif (recopt_tf)
      % this is a recognized option, parse it
      
      optdef = optdefs.(recopt_optname);
      
      optarg = {};
      if (isfield(seenopts, recopt_optname))
        optarg = seenopts.(recopt_optname);
      end
      
      % does this option take any argument?
      if (optdef.has_arg)
        i = i + 1;
        if (i > numel(argin))
          exception = MException('parse_opts:option_missing_arg', ...
            'Missing argument to option `%s''.', arg);
          return;
        end
        % the following is equivalent to   optarg = { optarg{:}, argin{i} };
        optarg = [ optarg , argin(i) ];
      end
      
      seenopts.(recopt_optname) = optarg;
      
      [seenopts args exception] = parse_implications(optdefs, mode_optname, seenopts, ...
                                                     args, optarg);
      
    else
      
      % We have an unrecognized argument. See if we allow arguments, otherwise it is an
      % error
      
      if (~optdefs.opt_allow_args)
        exception = MException('parse_opts:option_unrecognized', ...
          'Unrecognized argument %s', dispstr(arg));
        return;
      end
      
      args = [ args , { arg } ];
           
    end
    
    i = i + 1;
  end
end


function [seenopts args exception] = parse_implications(optdefs, optname, seenopts, args, ...
                                                    imploptval)
  
  exception = {};
  opt = optdefs.(optname);

  if (~opt.has_imply)
    % don't even care about implications
    return;
  end
  
  if (~isempty(opt.imply_if))
    if (~opt.imply_if(imploptval))
      % if 'imply_if' fails, don't imply.
      return;
    end
  end
  
  [imply_seenopts imply_args exception] = parse_seen_options(optdefs, optdefs.(optname).imply);
  if (isa(exception, 'MException'))
    return;
  end
  
  % make sure that the implied seen options are there, add them if necessary.
  seenoptnames = fieldnames(imply_seenopts);
  for i=1:numel(seenoptnames)
    sopt = seenoptnames{i};
    if (~isfield(seenopts, sopt))
      seenopts.(sopt) = imply_seenopts.(sopt); % this is already cell array
    elseif (optdefs.(sopt).is_boolean)
      % If this option was a switch, then keep a single boolean value and
      % report conflict in case.
      if (logical(imply_seenopts.(sopt)) ~= seenopts.(sopt))
        exception = MException('parse_opts:implication_conflict:bool', ...
             'Conflicting value for switch `%s'', implied by `%s''.', sopt, optname);
        return;
      end
    else
      % add all the implied values if necessary
      seenopts.(sopt) = merge_implications(seenopts.(sopt), imply_seenopts.(sopt), optname);
    end
    % If this is a non-repeatable option, then report value conflicts.
    % Note that all values have already been cumulated into `seenopts` as a cell array.
    if (~optdefs.(sopt).ok_repeat && numel(seenopts.(sopt)) > 1)
      % but allow if values are equal
      if (are_all_equal(seenopts.(sopt)))
        xyz = seenopts.(sopt);
        seenopts.(sopt) = xyz(1);
      else
        exception = MException('parse_opts:implication_conflict', ...
          'Conflicting value for `%s'', implied by `%s''.', sopt, optname);
      end
      return;
    end
  end

  % merge implied arguments if needed
  args = merge_implications(args, imply_args, optname);

end

function [newlist] = merge_implications(userlist, impliedlist, imploption)
  newlist = userlist;
  if (~iscell(newlist))
    newlist = { newlist };
  end
  for i=1:numel(impliedlist)
    item = impliedlist(i);
    if (~findorzero(cellfun(@(x) isequal(item, x), newlist)))
      %display(sprintf('Automatically adding %s, implied by %s', dispstr(item{1}), imploption));
      newlist = [ newlist item ];
    end
  end
end

function [tf] = are_all_equal(list)
  inequal_cmp_values = cellfun(@(x) ~isequal(x, list{1}), list(2:end), ...
    'UniformOutput', true);
  if (~any(inequal_cmp_values(:)))
    % yes all are equal
    tf = true;
  else
    tf = false;
  end
end


function [tf val] = is_list_item(optdefs, list, arg)
  
  %
  % Check whether arg is an item in list, used for looking up options.
  %
  % Will return false anyway if arg starts with 'opt_'.
  %
  % Will do case-sensitive check following optdefs.opt_case_sensitive.
  %
  
  tf = false;
  val = [];
  
  if (~ischarstring(arg))
    tf = false;
    return;
  end

  % loop all options, look for match
  for ii=1:numel(list)
    on = list{ii};
    if (numel(on) >= 4 && strcmp(on(1:4), 'opt_'))
      continue;
    end
    
    if (mystrcmp(on, arg, optdefs))
      tf = true;
      val = on;
      return;
    end
  end
  
  tf = false;
  return;
 
end

function [tf fname] = is_optstruct_field(optdefs, object, arg)
  
  %
  % Check whether arg is a structure field of object, used for looking up options.
  %
  % Will return false anyway if arg starts with 'opt_'.
  %
  % Will do case-sensitive check following optdefs.opt_case_sensitive.
  %
  
  tf = false;
  fname = '';
  
  if (~ischarstring(arg))
    tf = false;
    return;
  end
  if (optdefs.opt_case_sensitive)
    if (isfield(object, arg) && (numel(arg)<4 || ~strcmp(arg(1:4), 'opt_')))
      tf = true;
      fname = arg;
      return;
    end
    tf = false;
    return;
  end
  
  % loop all options, look for match
  onames = fieldnames(object);
  for ii=1:numel(onames)
    on = onames{ii};
    if (numel(on) >= 4 && strcmp(on(1:4), 'opt_'))
      continue;
    end
    
    if (mystrcmp(on, arg, optdefs))
      tf = true;
      fname = on;
      return;
    end
  end
  
  tf = false;
  return;
 
end

function [tf optname] = is_recognized_option(optdefs, arg)

  [tf optname] = is_optstruct_field(optdefs, optdefs, arg);
  
end


function [tf shortcut] = is_shortcut_option(optdefs, arg)
  
  [tf optname] = is_recognized_option(optdefs, arg);
  if (tf && optdefs.(optname).is_shortcut)
    tf = true;
    shortcut = optdefs.(optname).shortcut;
    return;
  end
  tf = false;
  shortcut = {};
  return;
  
end


function [tf optname modeval modeneedsarg] = is_mode_option(optdefs, arg)
  
  modeneedsarg = false;
  [tf optname] = is_recognized_option(optdefs, arg);
  if (tf)
    if (optdefs.(optname).is_mode)
      tf = true;
      modeval = optdefs.(optname).mode;
      return;
    end
    tf = false;
    modeval = [];
    return;
  end
  
  % test modes, see if arg is an exclusive mode arg
  [modetf modename] = is_optstruct_field(optdefs, optdefs.opt__modes, arg);
  if (modetf)
    tf = true;
    optname = modename;
    modeval = [];
    modeneedsarg = true;
    return;
  end
  
  tf = false;
  modeval = [];
  return;
end

function [tf optname boolval] = is_boolean_option(optdefs, arg)
  
  function [tf optname boolval] = test_prefix(arg, optdefs, prefix, thisboolval)
    if (numel(arg) > numel(prefix) && mystrcmp(arg(1:numel(prefix)), prefix, optdefs))
      [rec_tf rec_opt] = is_recognized_option(optdefs, arg((numel(prefix)+1):end));
      if (rec_tf && optdefs.(rec_opt).is_boolean)
        tf = true;
        optname = rec_opt;
        boolval = thisboolval;
        return;
      end
    end
    tf = false;
    optname = '';
    boolval = false;
    return;
  end
  
  [tf optname] = is_recognized_option(optdefs, arg);
  if (tf)
    if (optdefs.(optname).is_boolean)
      tf = true;
      boolval = true;
      return;
    end
    tf = false;
    boolval = false;
    return;
  end
  [tf optname boolval] = test_prefix(arg, optdefs, 'No', false);
  if (tf)
    return;
  end
  [tf optname boolval] = test_prefix(arg, optdefs, 'Without', false);
  if (tf)
    return;
  end
  [tf optname boolval] = test_prefix(arg, optdefs, '+', true);
  if (tf)
    return;
  end
  [tf optname boolval] = test_prefix(arg, optdefs, 'Yes', true);
  if (tf)
    return;
  end
  [tf optname boolval] = test_prefix(arg, optdefs, 'With', true);
  if (tf)
    return;
  end
  
  tf = false;
  optname = '';
  boolval = false;
  return;
  
end


function tf = mystrcmp(a, b, optdefs)
  if (optdefs.opt_case_sensitive)
    tf = strcmp(a,b);
    return;
  end
  tf = strcmpi(a,b);
  return;
end


function [ok tf] = looks_like_boolean(optdefs, arg)
  if (~ischarstring(arg))
    if (islogical(arg))
      ok = true;
      tf = logical(arg);
    elseif (isnumeric(arg) && (arg == 0 || arg == 1))
      ok = true;
      tf = logical(arg);
    else
      ok = false;
      tf = false;
    end
    return;
  else
    % is char string
    [ok tf] = parsestringbool(arg);
    return;
  end
end

function [ok tf] = parsestringbool(s)
  ok = true;
  if (~ischarstring(s))
    ok = false;
    tf = false;
    return;
  elseif (strcmpi(s,'f'))
    tf = false;
    return;
  elseif (strcmpi(s,'false'))
    tf = false;
    return;
  elseif (strcmpi(s,'n'))
    tf = false;
    return;
  elseif (strcmpi(s,'no'))
    tf = false;
    return;
  elseif (strcmpi(s,'off'))
    tf = false;
    return;
  elseif (strcmpi(s,'0'))
    tf = false;
    return;
  elseif (strcmpi(s,'1'))
    tf = true;
    return;
  elseif (strcmpi(s,'t'))
    tf = true;
    return;
  elseif (strcmpi(s,'true'))
    tf = true;
    return;
  elseif (strcmpi(s,'yes'))
    tf = true;
    return;
  elseif (strcmpi(s,'y'))
    tf = true;
    return;
  elseif (strcmpi(s,'on'))
    tf = true;
    return;
  elseif (strcmpi(s,'ok'))
    tf = true;
    return;
  else
    ok = false;
    tf = false;
    return;
  end
end


function tf = getbool(s)
  [ok tf] = parsestringbool(s);
  if (ok)
    return;
  elseif (isnumeric(s))
    tf = any(logical(s));
    return;
  elseif (islogical(s))
    tf = any(s);
    return;
  else
    error('parse_opts:getbool_unparsable_string', ...
      'Can''t parse supposedly boolean value %s', dispstr(s));
  end
end




function exception = do_call_argcheck(fncheck, val, optname)
  exception = [];

  tf = [];
  try
    [tf msg] = fncheck(val);
  catch err
    if (strcmp(err.identifier,'MATLAB:unassignedOutputs'))
      % fncheck was not meant to be called with 2 output vars, just retry below.
      tf = [];
    else
      % some other error
      rethrow(err);
    end
  end
  
  if (isempty(tf))
    % previous call failed, retry
    tf = fncheck(val);
    msg = 'argcheck failed';
  end
  
  % parse tf value
  
  if (~logical(tf))
    exception = ...
        MException('parse_opts:failed_argcheck', ...
                   'Invalid value `%s'' for option `%s'':  %s', dispstr(val), ...
                   optname, msg);
  end

end
            




function [ ok msg ] = do_argcheck_argtype(x, argtyp, optname)
  ok = false;
  if (strcmp(argtyp, 's'))
    ok = ischarstring(x) ;
    msg = 'Value is not a character string';
  elseif (strcmp(argtyp, 'n'))
    ok = isnumeric(x) ;
    msg = 'Value is not of numeric type';
  elseif (strcmp(argtyp, 'N'))
    ok = (isnumeric(x) && numel(x) == 1) ;
    msg = 'Value is not a single numeric value';
  elseif (strcmp(argtyp, 'I'))
    ok = (isnumeric(x) && numel(x) == 1 && x == floor(x)) ;
    msg = 'Value is not an integer';
  elseif (strcmp(argtyp, 'cell'))
    ok = iscell(x) ;
    msg = 'Value is not a cell array';
  elseif (strcmp(argtyp, 'fh'))
    ok = isa(x, 'function_handle') ;
    msg = 'Value is not a function handle';
  else
    error('parse_opts:optdefs:unknownargtype', ...
          'Unknown expected argument type `%s'' for option `%s''.', argtyp, optname);
  end
end
