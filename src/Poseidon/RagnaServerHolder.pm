###########################################################
# Poseidon server - Ragnarok Online server emulator
#
# This program is free software; you can redistribute it and/or 
# modify it under the terms of the GNU General Public License 
# as published by the Free Software Foundation; either version 2 
# of the License, or (at your option) any later version.
#
# Copyright (c) 2005-2006 OpenKore Development Team
###########################################################
# This class emulates a Ragnarok Online server.
# The RO client connects to this server. This server
# periodically sends a GameGuard query to the RO client,
# and saves the RO client's response.
###########################################################

# TODO:
# 1) make use of unpack strings to pack our packets depending on serverType
# 2) make plugin like connection algorithms for each serverType or 1 main algo on which serverTypes have hooks

package Poseidon::RagnaServerHolder;

use strict;
use Base::Server;
use base qw(Base::Server);
use Misc;
use Utils qw(binSize getCoordString timeOut getHex getTickCount);
use Poseidon::Config;
use FileParsers;
use Math::BigInt;

my %clientdata;

# Decryption Keys
my $enc_val1 = 0;
my $enc_val2 = 0;
my $enc_val3 = 0;
my $state    = 0;

sub new {
	my ($class, $port, $host, $number_of_clients, $first_ragna_port) = @_;
	my $self = $class->SUPER::new($port, $host);
	
	$self->{username_to_index} = {};
	$self->{index_to_username} = {};
	
	$self->{connected_num} = 0;
	
	$self->{clients_num} = $number_of_clients;
	
	$self->{clients_servers} = [];
	
	foreach my $server_index (0..($number_of_clients - 1)) {
		my $current_ragna_port = ($first_ragna_port + $server_index);
		
		my $server = Poseidon::RagnarokServer->new($current_ragna_port, $host, $self);
		
		push(@{$self->{clients_servers}}, $server);
		
		print "Ragnarok Online Server Opened port ".$current_ragna_port." for a client\n";
	}

	return $self;
}

sub find_bounded_client {
	my ($self, $username) = @_;
	if (exists $self->{username_to_index}{$username}) {
		return $self->{username_to_index}{$username};
	} else {
		return -1;
	}
}

sub find_free_client {
	my ($self) = @_;
	
	foreach my $server_index (0..$#{$self->{clients_servers}}) {
		my $server = $self->{clients_servers}[$server_index];

		next unless ($server->{client});
		next unless ($server->{client}->{connectedToMap});
		next if (exists $server->{client}->{boundUsername});
		return $server_index;
	}
	
	return -1;
}

sub bound_client {
	my ($self, $index, $username) = @_;
	my $client = $self->clients->[$index];
	$client->{boundUsername} = $username;
	$self->{username_to_index}{$username} = $index;
	$self->{index_to_username}{$index} = $username;
	print "Ragnarok Online Client of index: ".$index." was bounded to username: ".$username."\n";
}

sub onClientNew {
	my ($self, $client, $index) = @_;

	if ( $state == 0 ) {
		# Initialize Decryption
		$enc_val1 = 0;
		$enc_val2 = 0;
		$enc_val3 = 0;
	} else { 
		$state = 0;
	}
	
	$client->{challengeNum} = 0;
	
	if (exists $self->{index_to_username}{$client->{index}}) {
		$client->{boundUsername} = $self->{index_to_username}{$client->{index}};
		print "[RagnarokServer]-> Ragnarok Online client (".$client->{index}.") connected, it is bounded to username ".$client->{boundUsername}.".\n";
		
	} else {
		print "[RagnarokServer]-> Ragnarok Online client ($index) connected, it isn`t bounded yet.\n";	
	}
	
	$self->{connected_num}++;
}

sub onClientExit 
{
	my ($self, $client, $index) = @_;
	
	if (exists $self->{index_to_username}{$client->{index}}) {
		print "[RagnarokServer]-> Ragnarok Online client (".$client->{index}.") disconnected, it was bounded to username ".$client->{boundUsername}.".\n";
		
	} else {
		print "[RagnarokServer]-> Ragnarok Online client ($index) disconnected, it wasn`t bounded.\n";	
	}
	
	$self->{connected_num}--;
}

sub iterate {
	my ($self) = @_;
	$self->SUPER::iterate();
	
	foreach my $server_index (0..$#{$self->{clients_servers}}) {
		my $server = $self->{clients_servers}[$server_index];
		$server->iterate;
	}
}

1;

