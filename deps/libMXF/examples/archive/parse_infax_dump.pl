#!/usr/bin/perl -W

#
# $Id: parse_infax_dump.pl,v 1.1 2007/01/10 10:28:34 philipn Exp $
#
#  
# 
#  Copyright (C) 2006  Philip de Nier <philipn@users.sourceforge.net>
# 
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
# 
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
# 
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
# /
 

# 
# Parses and checks the Infax data dump D3.txt
#
# Usage: ./parse_infax_dump.pl D3.txt
#

use strict;


my @limits = 
(
    -1,     # don't care
    6,      # Format
    72,     # Prog title
    144,    # Ep title
    -2,     # TX date (check date format)
    8,      # Prog No
    1,      # Spool Status
    -2,     # Stock date  (check date format)
    29,     # Spool desc
    120,    # Memo
    -3,     # Duration (check duration format)
    14,     # Spool No
    14,     # Acc No
    -1,     # Missing (don't care)
    10,     # Cat info
);    


die "Missing D3 Infax dump filename" if (!defined $ARGV[0]);



open(INFAXDUMP, "<", $ARGV[0]) or die "failed to open Infax data file: $!\n";

my $lineIndex = 0;
while (my $line = <INFAXDUMP>)
{
    chomp($line); # remove newline
    
    # skip the first line with the headings
    if ($lineIndex > 0)
    {
        my $fieldIndex = 0;
        my @fields = split(/\|/, $line);
        foreach my $field (@fields)
        {
            # remove the mag prefix from the programme number
            if ($fieldIndex == 5 && $field =~ /^.*\:.*$/)
            {
                my ($magPrefix, $progno) = ($field =~ /^(.*)\:(.*)$/);
                
                # check mag prefix and programme number
                if (length $magPrefix == 0)
                {
                    print "zero length mag prefix for field '$field' ($fieldIndex) on line $lineIndex\n";
                }
                if (length $progno == 0)
                {
                    print "zero length prog number prefix for field '$field' ($fieldIndex) on line $lineIndex\n";
                }
                
                $field = $progno;
            }
            
            # check string size
            if ($limits[$fieldIndex] >= 0)
            {
                if (length($field) > $limits[$fieldIndex])
                {
                    print "limit exceeded for field '$field' ($fieldIndex) on line $lineIndex\n";
                }
            }
            # check date format
            elsif ($limits[$fieldIndex] == -2)
            {
                if (length($field) > 0 && $field !~ /^\d+\/\d+\/\d+\s.*$/)
                {
                    print "invalid date format for field '$field' ($fieldIndex) on line $lineIndex\n";
                }
            }
            # check duration format
            elsif ($limits[$fieldIndex] == -3)
            {
                if (length($field) > 0 && $field !~ /^\d+\:\d+\:\d+$/)
                {
                    print "invalid duration format for field '$field' ($fieldIndex) on line $lineIndex\n";
                }
            }
            
            $fieldIndex++;
        }
    }

    $lineIndex++;   
}

close(INFAXDUMP);


