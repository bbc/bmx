#!/usr/bin/perl -w

# $Id: PSE_html_summary.pl,v 1.1 2007/02/21 17:38:29 philipn Exp $
#
# Copyright (C) 2007  Jim Easterbrook <easter@users.sourceforge.net>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright notice,
#       this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the British Broadcasting Corporation nor the names
#       of its contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.


# read metadata
%data = ();
$metadata = "<table cellpadding=\"8\">\n";
while (<>)
{
  chomp;
  if (m|^(.+?:)\s*(.*)|)
  {
    $key = uc $1;
    $value = $2;
    $metadata .= "<tr><td><b>$key</b></td><td>$value</td></tr>\n";
    $data{$key} = $value;
  }
  else
  {
    last;
  }
}
$metadata .= "</table>";

# read summary
$summary = "<p><b>SUMMARY OF RESULTS:</b></p>\n";
$summary .= "<table border=\"1\" rules=\"all\" align=\"center\">\n";
$summary .= "<tr><th valign=\"top\">Type of violation</th>";
$summary .= "<th>Number of violations<br>(frames)</th></tr>\n";
while (<>)
{
  chomp;
  if (m|^(.+?):\s*(.*)|)
  {
    $key = $1;
    $value = $2;
    $summary .= "<tr><td>$key</td><td align=\"center\">$value</td></tr>\n";
  }
  else
  {
    last;
  }
}
$summary .= "</table>\n";
$summary .= "<p><b>Failed material should be re-analysed in its entirety";
$summary .= " for compliance after any changes.</b></p>\n";

# read detail list
$details = "<p align=\"center\" style=\"page-break-before: always\">";
$_ = <>;
chomp;
if (m|^Detail table threshold:\s*(\d*)|)
{
  $threshold = int $1;
  $details .= sprintf "<b>Detailed table of results".
                      " indicating %s exceeding %d (%3.1f) in level</b></p>\n",
                      $threshold >= 500 ? "violations" : "warnings",
                      $threshold, $threshold / 1000.0;
}
else
{
  $details .= "<b>Detailed table of results</b></p>\n";
}
$details .= "<table width=\"100%\">\n";
$details .= sprintf "<tr><td></td>";
$details .= sprintf "<td align=\"right\"><b>DATE OF ANALYSIS:</b> %s</td></tr>\n",
            $data{"DATE OF ANALYSIS:"};
$details .= sprintf "<tr><td><b>SPOOL NUMBER:</b> %s</td>",
            $data{"SPOOL NUMBER:"};
$details .= sprintf "<td align=\"right\"><b>DURATION:</b> %s</td></tr>\n",
            $data{"DURATION:"};
$details .= "</table>\n<br>\n";
$details .= "<table border=\"1\" rules=\"all\" cellpadding=\"4\">\n";
$details .= "<colgroup>\n";
$details .= "<colgroup>\n";
$details .= "<colgroup>\n";
$details .= "<colgroup width=\"12%\">\n";
$details .= "<colgroup width=\"12%\">\n";
$details .= "<colgroup width=\"12%\">\n";
$details .= "<colgroup>\n";
$details .= "<thead><tr><th>FRAME</th><th>VITC</th><th>LTC</th><th>RED</th>";
$details .= "<th>SPAT</th><th>LUM</th><th>X</th></tr></thead><tbody>\n";
$lastFrame = -1;
while (<>)
{
  chomp;
  if (m|(\d+)\s+([0-9:]+)\s+([0-9:]+)\s+(\d\.\d)\s+(\d\.\d)\s+(\d\.\d)\s+([X ])|)
  {
    $frame = $1;
    $vitc  = $2;
    $ltc   = $3;
    $red   = $4;
    $spat  = $5;
    $lum   = $6;
    $ext   = $7;
    if ($lastFrame >= 0 && $frame != $lastFrame + 1)
    {
      $details .= "<tr><td colspan=\"7\"></td></tr>\n";
    }
    $details .= sprintf "<tr><td align=\"right\">%s</td><td>%s</td>".
                        "<td>%s</td><td align=\"center\">%s</td>".
                        "<td align=\"center\">%s</td>".
                        "<td align=\"center\">%s</td><td>%s</td></tr>\n",
                        $frame, $vitc, $ltc, $red, $spat, $lum, $ext;
    $lastFrame = $frame;
  }
}
$details .= "</tbody></table>";

# write html
print <<END;
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN"
 "http://www.w3.org/TR/REC-html40/loose.dtd">
<html>
<head>
<title>TEST SUMMARY</title>
</head>
<body>
<h1 align="center">TEST SUMMARY</h1>
<p align="center">Results of analysis for compliance with ITC Guidance Notes
on Flashing Images and Regular Patterns in Television (July 2001) by
HardingFPA Flash and Pattern Analyser software</p>
$metadata
$summary
$details
</body>
</html>
END
