<?php
/*
 * Copyright (C) 2005-2012 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * MaNGOSsoap client example
 *
 * a simple example how to invoke commands using SOAP
 *
 * Make sure you enabled SOAP in your mangosd.conf!
 *  SOAP.Enabled = 1
 */

/*
 * $username and $password MUST be uppercase
 * account $username needs at least SEC_ADMINISTRATOR
 */
$username = 'ADMINISTRATOR';
$password = 'ADMINISTRATOR';

$host = "localhost";
$soapport = 7878;
$command = "server info";

$client = new SoapClient(NULL,
array(
    "location" => "http://$host:$soapport/",
    "uri" => "urn:MaNGOS",
    "style" => SOAP_RPC,
    'login' => $username,
    'password' => $password
));

try {
    $result = $client->executeCommand(new SoapParam($command, "command"));

    echo "Command succeeded! Output:<br />\n";
    echo $result;
}
catch (Exception $e)
{
    echo "Command failed! Reason:<br />\n";
    echo $e->getMessage();
}
?>
