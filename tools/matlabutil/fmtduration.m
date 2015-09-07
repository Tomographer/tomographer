function s = fmtduration(seconds)
% Format a number of seconds into a human-readable string
%
% formats a duration (given in seconds) into the format
%
%   '15 days, 12:08:01'
%
% and tries to round the info so that it is reasonable to display.
%
  
  assert(seconds >= 0, 'the duration must be positive!');
  

  if (seconds > 86400) % we're counting in days
    rest = mod(seconds, 86400);
    if (rest < 3600 || rest > (86400-3600) || round(seconds/86400) > 10)
      % rounding up to a day
      s = days_str(round(seconds/86400));
      return;
    end
    % otherwise, recursively format the rest
    s = sprintf('%s, %s', days_str(floor(seconds/86400)), time_str(rest));
    return;
  end
  
  s = time_str(seconds);
  return;
end

function s = days_str(days)
  if (days == 1)
    s = '1 day';
  else
    s = sprintf('%d days', days);
  end
end

function s = time_str(seconds)
  % less than a day: format in HH:MM:SS
  nhours = floor(seconds/3600);
  resthours = mod(seconds, 3600);
  nmins = floor(resthours/60);
  restmins = mod(resthours, 60);
  nsec = floor(restmins);
  
  s = sprintf('%02d:%02d:%02d', nhours, nmins, nsec);
end
