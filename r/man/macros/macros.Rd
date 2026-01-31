\newcommand{\citep}{\insertCite{#1}{moocore}}%
\newcommand{\citet}{\insertCite{#1;textual}{moocore}}%

% Macro to create link to an HTML vignette from a help page.
% arg 1 is the name of the vignette (without .html)
% arg 2 is the text to show on the link.
%% See also:
%% https://github.com/dmurdoch/rgl/blob/ec1e6cf237f91064a3e00b4627c6e39005779f83/man/macros/macros.Rd#L10
\newcommand{\HTMLVignette}{\href{https://multi-objective.github.io/moocore/r/articles/#1.html}{#2}}
