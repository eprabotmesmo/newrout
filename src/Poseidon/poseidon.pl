#!/usr/bin/env perl
###########################################################
# Poseidon server
#
# This program is free software; you can redistribute it and/or 
# modify it under the terms of the GNU General Public License 
# as published by the Free Software Foundation; either version 2 
# of the License, or (at your option) any later version.
#
# Copyright (c) 2005-2006 OpenKore Development Team
#
# Credits:
# isieo - schematic of XKore 2 and other interesting ideas
# anonymous person - beta-testing
# kaliwanagan - original author
# illusionist - bRO support
# Fr3DBr - bRO Update (El Dicastes++)
###########################################################

#package Poseidon;

use strict;
use FindBin qw($RealBin);
use lib "$RealBin/..";
use lib "$RealBin/../..";
use lib "$RealBin/../deps";
use Time::HiRes qw(time sleep);
use Poseidon::Config;
use Poseidon::RagnarokServer;
use Poseidon::QueryServer;
use Poseidon::ConnectServer;

use constant POSEIDON_SUPPORT_URL => 'http://wiki.openkore.com/index.php?title=Poseidon';
use constant SLEEP_TIME => 0.01;

our @servers;
our $connect_server;

sub initialize {
	# Loading Configuration
	Poseidon::Config::parse_config_file ("poseidon.txt", \%config);
	
	my $connect_port = $config{connectserver_port};
	my $connect_ip = $config{connectserver_ip};
	$connect_server = new Poseidon::ConnectServer($connect_port, $connect_ip, \@servers);
	print "Connect Server Ready At : " . $connect_server->getHost() . ":" . $connect_server->getPort() . "\n";
	
	my $ragna_ip = $config{ragnarokserver_ip};
	my $query_ip = $config{queryserver_ip};
	my $first_ragna_port = $config{ragnarokserver_first_port};
	my $first_query_port = $config{queryserver_first_port};
	my $number_of_servers = $config{number_of_servers};
	
	foreach my $server_index (1..$number_of_servers) {
		my $current_ragna_port = ($first_ragna_port + $server_index - 1);
		my $current_query_port = ($first_query_port + $server_index - 1);
		
		my $roServer = Poseidon::RagnarokServer->new($current_ragna_port, $ragna_ip, $server_index);
		print "Ragnarok Online Server (".$server_index.") Ready At : " . $roServer->getHost() . ":" . $roServer->getPort() . "\n";
		
		my $queryServer = Poseidon::QueryServer->new($current_query_port, $query_ip, $roServer, $server_index);
		print "Query Server (".$server_index.") Ready At : " . $queryServer->getHost() . ":" . $queryServer->getPort() . "\n";
		
		push(@servers, {ragnarok_server_port => $current_ragna_port, query_server_port => $current_query_port, ragnarok_server => $roServer, query_server => $queryServer});
	}
	
	print ">>> Poseidon 2.1 initialized <<<\n\n";
	print "Please read " . POSEIDON_SUPPORT_URL . "\n";
	print "for further instructions.\n";
}

sub __start {
	initialize();
	while (1) {
		$connect_server->iterate;
		foreach my $server_hash (@servers) {
			$server_hash->{ragnarok_server}->iterate;
			$server_hash->{query_server}->iterate;
		}
	}
}

__start() unless defined $ENV{INTERPRETER};
