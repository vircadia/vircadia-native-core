<?php
function send_voxels($inputFileName,$server,$port,$command) {
	$socketHandle = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);
	$serverIP = $server; 
	$serverSendPort = $port;

	$inputFile = fopen($inputFileName, "rb"); // these are text files
	$voxNum=0;
	while (!feof($inputFile) ) {
		// [1:'I'][2:num]...
		$netData = pack("cv",ord($command),$voxNum);

		$packetSize = 3; // to start
		while ($packetSize < 1450 && !feof($inputFile)) {
			$octets = fread($inputFile,1);
			if (feof($inputFile)) {
				echo "end of file\n";
				break;
			}
			$octets = (int)ord($octets);
			echo "read octets=$octets\n";

			$size = ceil($octets*3/8)+3;
			$fileData = fread($inputFile,$size);
			$size++; // add length of octet

			// ...[1:octets][n:xxxxx]
			$netData .= pack("c",$octets).$fileData;
			$packetSize+=$size;
			echo "sending adding octets=$octets size=$size to packet packetSize=$packetSize\n";
		}
		
		$result = socket_sendto($socketHandle, $netData, $packetSize, 0, $serverIP, $serverSendPort);
		echo "sent packet server=$serverIP port=$serverSendPort $voxNum size=$packetSize result=$result\n";
		usleep(20000); // 1,000,000 per second
		$voxNum++;
	}
	socket_close($socketHandle);
}

function send_zcommand($server,$port,$command) {
	$socketHandle = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);
	$serverIP = $server; 
	$serverSendPort = $port;

	// [1:'Z'][2:command][0]...
	$netData = pack("c",ord('Z'));
	$netData .= $command;
	$netData .= pack("c",0);

	$packetSize = 2+strlen($command);
	echo "sending packet server=$serverIP port=$serverSendPort size=$packetSize \n";
	$result = socket_sendto($socketHandle, $netData, $packetSize, 0, $serverIP, $serverSendPort);
	socket_close($socketHandle);
}

function testmode_send_voxels($server,$port) {
	echo "psych! test mode not implemented!\n";
}

$options = getopt("i:s:p:c:",array('testmode','zcommand:'));
//print_r($options);
//break;

if (empty($options['i']) && empty($options['zcommand'])) {
	echo "USAGE: sendvoxels.php -i 'inputFileName' -s [serverip] -p [port] -c [I|R] \n";
} else {
	$filename = $options['i'];
	$server = $options['s'];
	$port = empty($options['p']) ? 40106 : $options['p'];
	$command = empty($options['c']) ? 'S' : $options['c'];
	switch($command) {
		case 'S':
		case 'E':
		case 'Z':
			//$command is good
		break;
		default:
			$command='S';// insert by default!
	}

	if ($options['testmode']) {
		echo "TEST MODE Sending Voxels server:$server port:$port \n";
		testmode_send_voxels($server,$port);
	} else if ($options['zcommand'] && $command=='Z') {
		echo "Sending Z command to server:$server port:$port \n";
		send_zcommand($server,$port,$options['zcommand']);
	} else {
		echo "Sending Voxels file:$filename server:$server port:$port command:$command \n";
		send_voxels($filename,$server,$port,$command);
	}
}

?>