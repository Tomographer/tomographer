function [ fhandle ] = mynamedfigure(figname, figtitle, varargin)

% Single-time figure window creation, referenced by logical name
%
% Open a figure, assiciate to it a logical name (figname) and then
% when called a second time, re-open the same figure, not a new one.
% If the figure was closed in the meantime, then it is re-opened.
%
% fighandle = mynamedfigure('fig-abcVSxyz', 'abc vs xyz', ...)
%
% For this function to work properly, figures # 50+ must be available.
% Fine-tune with global variable mynfig_offset.
%
% Returns the handle to the figure that was focused.
%
% Any number of figure properties may be specified additionally as arguments
% that will be set() to the figure. Are set by default:
% 'WindowStyle' to 'Docked', 'Name' to figtitle, 'PaperType' to 'A4',
% 'NumberTitle' to 'off'.
%

  global mynfig_namelist;
  global mynfig_offset;
  
  % big offset to avoid clash with existing figures
  if (isempty(mynfig_offset))
    mynfig_offset = 50;
  end
  
  indices = [ ];
  
  if (numel(mynfig_namelist) > 0)
    indices = find(strcmp(mynfig_namelist, figname));
  end
  
  fh = -1;
  if (numel(indices) == 0)
    % create new figure handle ID, and register it
    fh = mynfig_offset+numel(mynfig_namelist)+1;
    mynfig_namelist = [ mynfig_namelist, {figname} ];
  else
    % indices(1) is the index of the name -> i+offset = directly the figure handle
    fh = mynfig_offset+indices(1);
  end
  
  % findobj() : ~ "list of open figures"
  % idea: find(in list of open figures, handle fh)
  % needsinit=true if not found.
  needsinit = (numel(find(not(findobj()-fh))) == 0);
  
  % ** open or focus the figure **
  figure(fh);
  
  if (nargin < 2)
    figtitle = figname;
  end
  
  % ** possibly initialize the figure **
  if (needsinit)
    try
      set(fh, 'WindowStyle', 'Docked');
      set(fh, 'NumberTitle', 'off');
      set(fh, 'Name', figtitle);
      set(fh, 'PaperType', 'A4');
      %set(fh, 'Renderer', 'OpenGL');
      % if the figure is not docked, give it a width & height.
      p = get(gcf, 'Position');
      p(3) = 800; p(4) = 400; % width and height
      set(gcf, 'Position', p);
      % custom properties
      for i=1:2:size(varargin,2)
        set(fh, varargin{i}, varargin{i+1});
      end
    catch exception
      display(['Failed to set some figure properties. You may be running in a ' ...
               'non-graphical system.']);
      display(['You will still be able to save your figure, eg. with saveas or export_fig.']);
    end
  end
  
  fhandle = fh;
end
