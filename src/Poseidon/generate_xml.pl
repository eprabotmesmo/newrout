#!/usr/bin/perl -w

use utf8;
use strict;
use FindBin qw($RealBin);
use lib "$RealBin/..";
use lib "$RealBin/../..";
use lib "$RealBin/../deps";
use Poseidon::Config;
use Poseidon::RagnarokServer;
use Poseidon::QueryServer;

Poseidon::Config::parse_config_file ("poseidon.txt", \%config);
	
my $ragna_ip = $config{ragnarokserver_ip};
my $query_ip = $config{queryserver_ip};
my $first_ragna_port = $config{ragnarokserver_first_port};
my $first_query_port = $config{queryserver_first_port};
my $number_of_servers = $config{number_of_servers};

open OUT, ">:utf8", "poseidon.xml" or die $!;

print OUT '<?xml version="1.0" encoding="euc-kr" ?>

<clientinfo>
	<servicetype>brazil</servicetype>
	<servertype>primary</servertype>
	<extendedslot></extendedslot>';

foreach my $server_index (1..$number_of_servers) {
	my $current_ragna_port = ($first_ragna_port + $server_index - 1);
	my $current_query_port = ($first_query_port + $server_index - 1);
	
	print OUT '
	<connection>
		<display>Poseidon ['.$current_ragna_port.']</display>
		<desc>None</desc>
		<address>'.$ragna_ip.'</address>
		<port>'.$current_ragna_port.'</port>
		<version>1</version>
	</connection>';
}

print OUT '
</clientinfo>';

close(OUT);

print "Finished !\n";

system("pause");