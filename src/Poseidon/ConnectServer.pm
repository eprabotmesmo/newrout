###########################################################
# Poseidon server - OpenKore communication channel
#
# This program is free software; you can redistribute it and/or 
# modify it under the terms of the GNU General Public License 
# as published by the Free Software Foundation; either version 2 
# of the License, or (at your option) any later version.
#
# Copyright (c) 2005-2006 OpenKore Development Team
###########################################################
package Poseidon::ConnectServer;

use strict;
use Scalar::Util;
use Base::Server;
use Bus::MessageParser;
use Bus::Messages qw(serialize);
use Poseidon::RagnarokServer;
use base qw(Base::Server);
use Plugins;

my $CLASS = "Poseidon::ConnectServer";

sub new {
	my ($class, $port, $host, $roServer) = @_;
	my $self = $class->SUPER::new($port, $host);
	
	$self->{"server"} = $roServer;
	
	$self->{"$CLASS queue"} = [];
	
	print "[PoseidonServer]-> Connect server created\n";
	
	return $self;
}

sub process {
	my ($self, $client, $ID, $args) = @_;

	if ($ID ne "Poseidon Connect") {
		$client->close();
		return;
	} elsif (!$args->{username}) {
		print "Username is needed \n";
		return $args->{auth_failed};
	}

	print "[PoseidonServer]-> Received connection request from bot client (" . $args->{username} . ")\n";
	
	my $index = $self->getPortForClient($client, $args->{username});
	
	my %request = (
		client => $client,
		username => $args->{username},
		index => $index
	);

	Scalar::Util::weaken($request{client});
	push @{$self->{"$CLASS queue"}}, \%request;
}

sub getPortForClient {
	my ($self, $client, $username) = @_;
	my $index;
	
	$index = $self->{"server"}->find_bounded_client($username);
	if ($index != -1) {
		return $index;
	}
	
	$index = $self->{"server"}->find_free_client($username);
	if ($index != -1) {
		$self->{"server"}->bound_client($index, $username);
		return $index;
	}
	
	return -1;
}

sub onClientNew {
	my ($self, $client, $index) = @_;
	$client->{"$CLASS parser"} = new Bus::MessageParser();
	print "[PoseidonServer]-> Bot Client Connected to connect server: " . $client->getIndex() . "\n";
}

sub onClientExit {
	my ($self, $client, $index) = @_;
	print "[PoseidonServer]-> Bot Client Disconnected from connect server: " . $client->getIndex() . "\n";
}

sub onClientData {
	my ($self, $client, $msg) = @_;

	print "[PoseidonServer]-> Bot Client sent data to connect server: " . $client->getIndex() . "\n";
	
	my ($ID, $args);

	my $parser = $client->{"$CLASS parser"};
	
	$parser->add($msg);
	
	while ($args = $parser->readNext(\$ID))
	{
		$self->process($client, $ID, $args);
	}
}

sub iterate {
	my ($self) = @_;
	my ($server, $queue);

	$self->SUPER::iterate();
	$server = $self->{"$CLASS server"};
	$queue = $self->{"$CLASS queue"};
	
	while (@{$queue} > 0) {
		my $request = shift(@{$queue});
		
		if ($request->{index} == -1) {
			print "[PoseidonServer]-> Denied connection to character ". $request->{username} . " because there was no free client\n";
		} else {
			print "[PoseidonServer]-> Allowing connection to character ". $request->{username} . " on client index ". $request->{index} . "\n";
		}
		
		my ($data, %args);
		$args{packet} = $request->{index};
		$data = serialize("Poseidon Reply", \%args);
		$request->{client}->send($data);
		$request->{client}->close();
	}
}

1;
