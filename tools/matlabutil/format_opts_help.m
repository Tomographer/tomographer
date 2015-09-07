function s = format_opts_help(fn, parsed_optdefs)
% format_opts_help Format a help message corresponding to the given option defintions.
%
% [stk I] = dbstack(1);
% s = format_opts_help(stk(1), optdefs); % store help message in s
% format_opts_help([], optdefs); % display to the user
%
% Note that optdefs is not your declared optdefs, but rather it is the opts.opt_optdefs
% returned by parse_opts(). This is because parse_opts() sets some additional helper
% values and flags in optdefs which are needed by format_opts_help().
%
  ;

  % first, include the function help text itself
  if (isfield(fn, 'file'))
    s = [sprintf('\n') help(fn.file)];
  elseif (ischarstring(fn))
    s = [sprintf('\n') help(fn)];
  else
    s = sprintf('\n<Unknown Function>\n');
  end

  s = [s sprintf('Possible Options:\n\n')];
  
  optnames = fieldnames(parsed_optdefs);
  
  optexclmodes = parsed_optdefs.opt__modes;

  % which exclusive mode options we have already fully documented
  mode_excl_done = {};
  
  for j=1:numel(optnames)
    
    optname = optnames{j};

    if ((numel(optname) > 4) && strcmp(optname(1:4), 'opt_'))
      % internal opt_ flag or meta-option
      continue;
    end
    
    % document this option
    opt = parsed_optdefs.(optname);

    % simple option with argument
    if (opt.has_arg)
      if (opt.is_boolean)
        optsig = ['''[No|Yes|With|Without]' optname ''' [true/false]'];
      else
        optsig = sprintf('''%s'' %s', optname, argtypename(opt.arg, opt.allow_empty_arg));
      end
      if opt.has_default
        optsig = [optsig '  (default=' dispstr(opt.default) ')'];
      end
      if opt.ok_repeat
        optsig = [optsig '  [may be repeated]'];
      end
      s = [s sprintf('  %s\n', optsig)];
      if (isfield(opt, 'help'))
        s = [s do_wrap_text(opt.help, 80, '    ', '    ')]; % includes final '\n'
      end
    elseif (opt.is_mode && opt.is_exclusive_mode)
      if (any(strcmp(opt.mode, mode_excl_done)))
        continue; % already did this exclusive mode
      end
      % add this mode to the ones we have documented
      mode_excl_done = [mode_excl_done, {opt.mode}];
      %
      thisexclmode = optexclmodes.(opt.mode);
      % and document this mode
      s = [s sprintf('  [''%s''] <MODE> ;  where <MODE> is one of:\n', opt.mode)];
      for kj=1:numel(thisexclmode.list)
        thismodeopt = thisexclmode.list{kj};
        head = ['      ''' thismodeopt ''''];
        if isfield(parsed_optdefs.(thismodeopt), 'help')
          modeopthelp = parsed_optdefs.(thismodeopt).help;
        else
          modeopthelp = '';
        end
        if (thisexclmode.has_default_mode && strcmp(thisexclmode.default_mode, ...
                                                    thismodeopt))
          modeopthelp = sprintf('%s (the default)', modeopthelp);
        end
        if numel(modeopthelp)
          head = [head ' : '];
        end
        s = [s do_wrap_text(modeopthelp, 80, '        ', head)];
      end
    elseif (opt.is_mode)
      % non-exclusive mode
      s = [s sprintf('  ''%s''\n', optname)];
      if (isfield(opt, 'help'))
        s = [s do_wrap_text(opt.help, 80, '    ', '    ')]; % includes final '\n'
      end
    elseif (opt.is_shortcut)
      s = [s sprintf('  ''%s''\n', optname)];
      s = [s sprintf('    (shortcut) --> %s\n', dispstr(opt.shortcut))];
      if (isfield(opt, 'help'))
        s = [s do_wrap_text(opt.help, 80, '    ', '    ')]; % includes final '\n'
      end
    end
    if (opt.has_imply)
      s = [s sprintf('    Implies: %s\n', dispstr(opt.imply))];
    end
    s = [s sprintf('\n')];
  end
  
  if (nargout < 1)
    disp(s);
  end
end


function argnt = argtypename(arg, allow_empty_arg)
  arg2 = arg(find(arg~='*')); % remove any star
  if (strcmp(arg2, 's'))
    argnt = '<string>';
  elseif (strcmp(arg2, 'n'))
    argnt = '<numeric>';
  elseif (strcmp(arg2, 'N'))
    argnt = '<number>';
  elseif (strcmp(arg2, 'I'))
    argnt = '<integer>';
  elseif (strcmp(arg2, 'cell'))
    argnt = '<cell array>';
  elseif (strcmp(arg2, 'fh'))
    argnt = '<function handle>';
  elseif (strcmp(arg2, 'any'))
    argnt = '<ARGUMENT>';
  else
    argnt = '<UNKNOWN TYPE>';
  end
  if (allow_empty_arg)
    argnt = [argnt ' or []'];
  end
end


function s = do_wrap_text(x, width, prefix, prefixfirst)
  s = '';
  first = true;
  theprefix = prefixfirst;
  x = strtrim(x);
  if (isempty(x))
    % make sure head (prefixfirst) is always produced
    s = [prefixfirst sprintf('\n')];
    return;
  end
  while (~isempty(x))
    chunklen = min(width-numel(theprefix),numel(x));
    s = [s sprintf('%s%s\n', theprefix, x(1:chunklen))];
    if (first)
      first = false;
      theprefix = prefix;
    end
    if (chunklen==numel(x))
      x = [];
    else
      x = strtrim(x(chunklen+1:end));
    end
  end
end
