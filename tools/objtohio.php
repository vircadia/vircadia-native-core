<?php
// Parse/Read OBJ file
//
// 	USAGE: php objtohio.php -i 'inputFileName' -o 'outputFileName' -m [blocks|wireframe|surface] -s [voxel size]
//  NOTE:  Depending on the size of the model, it might take a lot of memory, so you will likely need to include
//         the -d memory_limit=4096M option
//
global $lastmsg;
$lastmsg='';

function echoProgress($msg,$keep=false) {
	global $lastmsg;
	for($bs=0;$bs<strlen($lastmsg);$bs++) {
		echo chr(8);
	}
	for($bs=0;$bs<strlen($lastmsg);$bs++) {
		echo ' ';
	}
	for($bs=0;$bs<strlen($lastmsg);$bs++) {
		echo chr(8);
	}
	echo $msg;
	if (!$keep)
		$lastmsg=$msg;
	else
		$lastmsg='';
}


class Vertice
{
	public $x;
	public $y;
	public $z;
}

class Voxel
{
	public $material_name;
	public $x;
	public $y;
	public $z;
	public $s;
}

global $renderMode;
$renderMode = 'surface';

global $minVoxelSize;
$minVoxelSize = 0.003; // default

function findVoxelsOfPolygon($vertices,$s=false,$material='default') {
	//echo "findVoxelsOfPolygon() vertices=".print_r($vertices,1);

	global $renderMode;
	switch ($renderMode)
	{
		case 'blocks':
			return polygonAsBlocks($vertices,$s,$material);
		case 'wireframe':
			//echo "findVoxelsOfPolygon() calling... polygonAsWireFrame() vertices=".print_r($vertices,1);
			return polygonAsWireFrame($vertices,$s,$material);
		default:
		case 'surface':
			return polygonAsSurface($vertices,$s,$material);
	}
}

function polygonAsWireFrame($vertices,$s=false,$material='default')
{
	if ($s === false) {
		global $minVoxelSize;
		$s = $minVoxelSize;
	}
	//echo "polygonAsWireFrame()\n";
	//echo "polygonAsWireFrame() vertices=".print_r($vertices,1);

	$maxNdx = count($vertices);
	//echo "***** polygonAsWireFrame() maxNdx=$maxNdx\n";
	$voxels = array();
	for ($ndx = 0; $ndx < $maxNdx; $ndx++)
	{
		$v1 = $vertices[$ndx];
		$v2 = ($ndx < ($maxNdx-1)) ? $vertices[$ndx+1] : $vertices[0];
		$lineVoxels = voxelLine($v1->x,$v1->y,$v1->z,$v2->x,$v2->y,$v2->z,$s,$material);
		//echo "polygonAsWireFrame() lineVoxels=".print_r($lineVoxels,1);
		$voxels = array_merge($voxels,$lineVoxels);
	}
	return $voxels;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Find the smallest Voxel of a random polygon.
// 		find minimums of X,Y,Z
//		find maximums of X,Y,Z
//		find lengths of max-mins for X,Y,Z
//	Voxel Point = (minX, minY, minZ,longest side)
function polygonAsBlocks($vertices,$s=false,$material='default')
{
	if ($s === false) {
		global $minVoxelSize;
		$s = $minVoxelSize;
	}

	//echo "polygonAsBlocks()\n";
	$minX = false;
	$minY = false;
	$minZ = false;
	$maxX = false;
	$maxY = false;
	$maxZ = false;
	foreach ($vertices as $vertex)
	{
		if ($minX === false || $vertex->x < $minX)
			$minX = $vertex->x;

		if ($minY === false || $vertex->y < $minY)
			$minY = $vertex->y;

		if ($minZ === false || $vertex->z < $minZ)
			$minZ = $vertex->z;

		if ($maxX === false || $vertex->x > $maxX)
			$maxX = $vertex->x;

		if ($maxY === false || $vertex->y > $maxY)
			$maxY = $vertex->y;

		if ($maxZ === false || $vertex->z > $maxZ)
			$maxZ = $vertex->z;
	}
	$longestSide = max($maxX-$minX,$maxY-$minY,$maxZ-$minZ);
	$shortestSide = max(0.0005,min($maxX-$minX,$maxY-$minY,$maxZ-$minZ));

//printf("findVoxelsOfPolygon() shortestSide=$shortestSide\n");
//printf("findVoxelsOfPolygon() minX=$minX, maxX=$maxX\n");
//printf("findVoxelsOfPolygon() minY=$minY, maxY=$maxY\n");
//printf("findVoxelsOfPolygon() minZ=$minZ, maxZ=$maxZ\n");
	
	$voxels = array();
	// Could we???
	// 		use the shortest side as our voxel unit, and build up
	// 		the partial volume from those blocks....???
	$x = $minX;
	while ($x <= $maxX)
	{
//printf("findVoxelsOfPolygon() X loop x=$x, y=$y, z=$z\n");
		$y = $minY;
		while($y <= $maxY)
		{
//printf("findVoxelsOfPolygon() x=$x, y=$y, z=$z\n");
			$z = $minZ;
			while ($z <= $maxZ)
			{
//printf("findVoxelsOfPolygon() x=$x, y=$y, z=$z, shortestSide=$shortestSide\n");
				$voxel = new Voxel();
				$voxel->x = (float)$x;
				$voxel->y = (float)$y;
				$voxel->z = (float)$z;
				$voxel->s = (float)$shortestSide;
				$voxels[]=$voxel;
				
				$z+=$shortestSide;
			}
			$y+=$shortestSide;
		}
		$x+=$shortestSide;
	}
	return $voxels;
}

// $scaleU is the scale in universal coordinates that the voxels will
// be scaled to fill. we use 1 by default
function rescaleVoxels($voxels,$scaleU=1)
{
	echo "rescaleVoxels()...\n";
	$minX = false;
	$minY = false;
	$minZ = false;
	$maxX = false;
	$maxY = false;
	$maxZ = false;
	
	$vc=0;

	foreach ($voxels as $voxel)
	{
		$vc++;
		echoProgress("searching for bounds voxel:$vc");
		
		//print_r($voxel);
		if ($minX === false || $voxel->x < $minX)
		{
			//echo "found new minX!\n";
			$minX = $voxel->x;
		}

		if ($minY === false || $voxel->y < $minY)
		{
			//echo "found new miny!\n";
			$minY = $voxel->y;
		}

		if ($minZ === false || $voxel->z < $minZ)
		{
			//echo "found new minZ!\n";
			$minZ = $voxel->z;
		}

		if ($maxX === false || ($voxel->x+$voxel->s) > $maxX)
		{
			$maxX = ($voxel->x+$voxel->s);
		}

		if ($maxY === false || ($voxel->y+$voxel->s) > $maxY)
		{
			//echo "found new maxY!\n";
			$maxY = ($voxel->y+$voxel->s);
		}

		if ($maxZ === false || ($voxel->z+$voxel->s) > $maxZ)
		{
			//echo "found new maxZ!\n";
			$maxZ = ($voxel->z+$voxel->s);
		}
	}
	echoProgress("Done searching for bounds...\n",true);

	echo "minX=$minX, minY=$minY, minZ=$minZ\n";
	echo "maxX=$maxX, maxY=$maxY, maxZ=$maxZ\n";
	$widthX = $maxX-$minX;
	$widthY = $maxY-$minY;
	$widthZ = $maxZ-$minZ;
	$widthU = max($widthX,$widthY,$widthZ);

	echo "widthX=$widthX, widthY=$widthY, widthZ=$widthZ, widthU=$widthU\n";

	$vc=0;
	foreach ($voxels as $voxel)
	{
		$vc++;
		echoProgress("Scaling voxel:$vc");
		
		$voxel->x = $widthX ? (($voxel->x - $minX)/$widthU)*$scaleU : 0;
		$voxel->y = $widthY ? (($voxel->y - $minY)/$widthU)*$scaleU : 0;
		$voxel->z = $widthZ ? (($voxel->z - $minZ)/$widthU)*$scaleU : 0;
		$voxel->s = ($voxel->s/$widthU)*$scaleU;
	}
	echoProgress("Done Rescaling Voxels!\n",true);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Function:    voxelPolygon()
// Description: Given an array of universal points forming a polygon in 3D space, this
//				will return an array of voxels that fill the interior of this polygon
//              The voxels will match size s. The input values x,y,z range 0.0 <= v < 1.0
// To Do:		Currently, this will only work on convex polygons. If the polygon is 
//				concave, we need to partition it into smaller convex polygons.
// Complaints:  Brad :)
function polygonAsSurface($vertices,$s=false,$material='default') {

	if ($s === false) {
		global $minVoxelSize;
		$s = $minVoxelSize;
	}

	//echo "polygonAsSurface()\n";
	//echoProgress("polygonAsSurface()");
	

	//echo "polygonAsSurface() vertices=".print_r($vertices,1);

	$voxels = array();
	// starting at v[1]
	//   for each vertice v
	//	   for N over v[v+1] to v[v+2]
	//	  	draw a line from v to v[v+1~v+2:N]
	//  NOTE: this will result in several overlapping/duplicate voxels, so discard extras!!
	//  ALSO NOTE: this will only work on convex polygons.
	// If the polygon is concave, we need to partition it into smaller convex polygons
	$count = count($vertices);
	$vO = $vertices[0];
	//echo "polygonAsSurface() count=$count\n";
	//echo "polygonAsSurface() vO=".print_r($vO,1);

	for($n = 1; $n < $count; $n++)
	{
		$nP1 = ($n+1)%$count;
		//echo "polygonAsSurface() n=$n nP1=$nP1\n";

		$vA = $vertices[$n];
		$vB = $vertices[$nP1];

		//echo "polygonAsSurface() vA=".print_r($vA,1);
		//echo "polygonAsSurface() vB=".print_r($vB,1);

		//echo "polygonAsSurface() ... voxelLine({$vA->x},{$vA->y},{$vA->z},{$vB->x},{$vB->y},{$vB->z},$s,$material)\n";
		
		$lAB = voxelLine($vA->x,$vA->y,$vA->z,$vB->x,$vB->y,$vB->z,$s,$material);

		//echo "**** polygonAsSurface() count(lAB)=".count($lAB)."\n";

		$lc=0;
		foreach($lAB as $vAB)
		{
			//echo ">>> polygonAsSurface() vAB=".print_r($vAB,1);
			//echo ">>> polygonAsSurface() ... voxelLine({$vO->x},{$vO->y},{$vO->z},{$vAB->x},{$vAB->y},{$vAB->z},$s,$material)\n";
			
			$lOAB = voxelLine($vO->x,$vO->y,$vO->z,$vAB->x,$vAB->y,$vAB->z,$s,$material);
			$lc++;
			//echoProgress("polygonAsSurface()... voxelLine() $lc");

			//echo ">>> polygonAsSurface() count(lOAB)=".count($lOAB)."\n";
			$voxels = array_merge($voxels,$lOAB);
		}
	}
	//echo "polygonAsSurface() count(voxels)=".count($voxels)."\n";
	//echoProgress("polygonAsSurface() count(voxels)=".count($voxels));
	return $voxels;
}


//////////////////////////////////////////////////////////////////////////////////////////
// Function:    voxelLine()
// Description: Given two universal points with location x1,y1,z1 and x2,y2,z2 this will
//              return an array of voxels that connect these two points with a straight
//              line. The voxels will match size s.
//              The input values x,y,z range 0.0 <= v < 1.0
// Complaints:  Brad :)
function voxelLine($x1,$y1,$z1,$x2,$y2,$z2,$s,$material='default')
{
	$voxels = array();
	$xV = $x2-$x1;
	$yV = $y2-$y1;
	$zV = $z2-$z1;
	$mV = max(abs($xV),abs($yV),abs($zV)); // max vector, which is the one we'll iterate accoss!
	$its = round($mV/$s);

	/**
	echo "voxelLine() xV=$xV\n";
	echo "voxelLine() yV=$yV\n";
	echo "voxelLine() zV=$zV\n";
	echo "voxelLine() mV=$mV\n";
	echo "voxelLine() its=$its\n";
	**/
	
	for ($i = 0; $i < $its; $i++)
	{
		//echo "voxelLine() loop i=$i\n";
		$x = $x1+(($xV/$its)*$i);
		$y = $y1+(($yV/$its)*$i);
		$z = $z1+(($zV/$its)*$i);
		$voxel = new Voxel();
		$voxel->x=$x;
		$voxel->y=$y;
		$voxel->z=$z;
		$voxel->s=$s;
		$voxel->material_name=$material;
		$voxels[] = $voxel;
	}
	return $voxels;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Function:    pointToVoxel()
// Description: Given a universal point with location x,y,z this will return the voxel
//              voxel code corresponding to the closest voxel which encloses a cube with
//              lower corners at x,y,z, having side of length S.  
//              The input values x,y,z range 0.0 <= v < 1.0
// TO DO:       This code is not very DRY. It should be cleaned up to be DRYer.
// IMPORTANT:   The voxel is returned to you a buffer which you MUST delete when you are
//              done with it.
// Usage:       
//                  unsigned char* voxelData = pointToVoxel(x,y,z,s,red,green,blue);
//                  tree->readCodeColorBufferToTree(voxelData);
//                  delete voxelData;
//
// Complaints:  Brad :)
global $maxOctet,$minOctet;
$maxOctet=0;
$minOctet=100;
function pointToVoxel($x,$y,$z,$s,$r=false,$g=false,$b=false) 
{
    $xTest = $yTest = $zTest = $sTest = 0.5;

    // First determine the voxelSize that will properly encode a 
    // voxel of size S.
    $voxelSizeInBits = 0;
    while ($sTest > $s) {
        $sTest /= 2.0;
        $voxelSizeInBits+=3;
    }

	$voxelSizeInBytes = round(($voxelSizeInBits/8)+1);
	$voxelSizeInOctets = round($voxelSizeInBits/3);
	$voxelBufferSize = $voxelSizeInBytes+1+3; // 1 for size, 3 for color

	if ($voxelSizeInOctets < 1)
	{
		echo "************ WTH!! voxelSizeInOctets=$voxelSizeInOctets s=$s sTest=$sTest ************\n";
	}
	
	// for debugging, track our largest octet
	global $maxOctet,$minOctet;
	//echo "voxelSizeInOctets=$voxelSizeInOctets\n";
	$maxOctet = max($maxOctet,$voxelSizeInOctets);
	$minOctet = min($minOctet,$voxelSizeInOctets);
	if ($minOctet < 1)
	{
		echo "************ WTH!! minOctet=$minOctet\n";
	}
	//echo "maxOctet=$maxOctet\n";

    // allocate our resulting buffer
    $voxelOut = array();
    
    // first byte of buffer is always our size in octets
    $voxelOut[0]=$voxelSizeInOctets;

    $sTest = 0.5;  // reset sTest so we can do this again.

    $byte = 0; // we will be adding coding bits here
    $bitInByteNDX = 0; // keep track of where we are in byte as we go
    $byteNDX = 1; // keep track of where we are in buffer of bytes as we go
    $octetsDone = 0;

    // Now we actually fill out the voxel code
    while ($octetsDone < $voxelSizeInOctets) {
    
		//echo "coding octet: octetsDone=$octetsDone voxelSizeInOctets=$voxelSizeInOctets\n";

        if ($x > $xTest) { 
            //<write 1 bit>
            $byte = ($byte << 1) | true;
            $xTest += $sTest/2.0; 
        }
        else { 
            //<write 0 bit;>
            $byte = ($byte << 1) | false;
            $xTest -= $sTest/2.0; 
        }
        $bitInByteNDX++;
        // If we've reached the last bit of the byte, then we want to copy this byte
        // into our buffer. And get ready to start on a new byte
        if ($bitInByteNDX > 7)
        {
			//printf("xTest: got 8 bits, write to array byteNDX=$byteNDX  byte=[%08b]\n",$byte);
            $voxelOut[$byteNDX]=$byte;
            $byteNDX += 1;
            $bitInByteNDX=0;
            $byte=0;
        }

        if ($y > $yTest) { 
            //<write 1 bit>
            $byte = ($byte << 1) | true;
            $yTest += $sTest/2.0; 
        }
        else { 
            //<write 0 bit;>
            $byte = ($byte << 1) | false;
            $yTest -= $sTest/2.0; 
        }
        $bitInByteNDX++;
		//echo "coding octet: octetsDone=$octetsDone voxelSizeInOctets=$voxelSizeInOctets bitInByteNDX=$bitInByteNDX\n";

        // If we've reached the last bit of the byte, then we want to copy this byte
        // into our buffer. And get ready to start on a new byte
        if ($bitInByteNDX > 7)
        {
			//printf("yTest: got 8 bits, write to array byteNDX=$byteNDX  byte=[%08b]\n",$byte);
            $voxelOut[$byteNDX]=$byte;
            $byteNDX += 1;
            $bitInByteNDX=0;
            $byte=0;
        }

        if ($z > $zTest) { 
            //<write 1 bit>
            $byte = ($byte << 1) | true;
            $zTest += $sTest/2.0; 
        }
        else { 
            //<write 0 bit;>
            $byte = ($byte << 1) | false;
            $zTest -= $sTest/2.0; 
        }
        $bitInByteNDX++;
        // If we've reached the last bit of the byte, then we want to copy this byte
        // into our buffer. And get ready to start on a new byte
        if ($bitInByteNDX > 7)
        {
			//printf("zTest: got 8 bits, write to array byteNDX=$byteNDX  byte=[%08b]\n",$byte);
            $voxelOut[$byteNDX]=$byte;
            $byteNDX += 1;
            $bitInByteNDX=0;
            $byte=0;
        }

        $octetsDone++;
        $sTest /= 2.0;
    } 

	//printf("done with bits: byteNDX=$byteNDX bitInByteNDX=$bitInByteNDX\n");

    // If we've got here, and we didn't fill the last byte, we need to zero pad this
    // byte before we copy it into our buffer.
    if ($bitInByteNDX > 0 && $bitInByteNDX < 8)
    {
        // Pad the last byte
        while ($bitInByteNDX <= 7)
        {
            $byte = ($byte << 1) | false;
            $bitInByteNDX++;
        }
        
        // Copy it into our output buffer
		//printf("padding: array byteNDX=$byteNDX byte=[%08b]\n",$byte);
        $voxelOut[$byteNDX]=$byte;
		$byteNDX += 1;
    }
    // copy color data
    if ($r !== false)
    {
		//printf("color: array byteNDX=$byteNDX red=$r [%08b] green=$g [%08b] blue=$b [%08b]\n",$r,$g,$b);
	}
    $voxelOut[$byteNDX]=$r;
    $voxelOut[$byteNDX+1]=$g;
    $voxelOut[$byteNDX+2]=$b;

    return $voxelOut;
}

function dumpVoxelCode($voxelCode)
{	
	foreach($voxelCode as $byte)
	{
		printf("[%08b]",$byte);
	}
	echo "\n";
}

class Material
{
	public $name;
	public $red;
	public $green;
	public $blue;
}

function randomColorValue($miniumum) {
    return $miniumum + (rand() % (255 - $miniumum));
}

global $all_voxels;
global $all_voxel_codes;

$all_voxels = array();
function add_voxels($voxels)
{
	global $all_voxels;
	// users can pass an array or a single voxel,
	// we're rather work on arrays, so conver a single
	// voxel into an array
	if (!is_array($voxels))
	{
		$voxels = array($voxels);
	}

//echo "add_voxels()...".print_r($voxels,1);

	foreach ($voxels as $voxel)
	{
		// we ignore color here!
		$voxelCode = pointToVoxel($voxel->x,$voxel->y,$voxel->z,$voxel->s,false,false,false);
		$voxelCode = implode(',',$voxelCode);
		$all_voxels[$voxelCode]=$voxel;
	}
//	echo "all_voxels count=".count($all_voxels)."\n";
}


function convertObjFile($inputFileName,$outputFileName)
{
	$inputFile = fopen($inputFileName, "rt"); // these are text files
	
	$fCount=0;
	
	$default_material = new Material();
	$default_material->name  = 'default';
	$default_material->red   = 240;
	$default_material->green = 240;
	$default_material->blue  = 240;
	$current_material = $default_material;
	$materials = Array('default'=>$default_material);
	
	$vertices = Array();
	$voxels = Array();

	// Note: AFAIK we can't be certain that all the vertices are included
	// at the start of the file before faces. So we need:
	//	1) scan the entire file, and pull out all the vertices. 
	//	2) Then we can resample that set of vertices to match our coordinate system. 
	//	3) THEN we can process the faces to make voxels
	$filePasses = array('v','f');
	$commands = array('v'=>array('v','f','fill_from'),'f'=>array('f','usemtl','material'));
	foreach ($filePasses as $pass)
	{
		fseek($inputFile,0); // always rewind at the start of each pass
		// Before we do the face pass, resample the vertices
		if ($pass == 'f')
		{
			// now we actually need to rescale our voxels to fit in a 0-1 coordinate space

			echo "vertices PRE-rescale:\n";
			//print_r($vertices);

			rescaleVoxels($vertices);

			echo "vertices POST-rescale:\n";
			//print_r($vertices);
		}
		
		while (!feof($inputFile) ) 
		{
			$line_of_text = fgets($inputFile);
			$line_of_text = ereg_replace('[[:blank:]]+', ' ', $line_of_text);
			$line_of_text = trim(str_replace('\n', '', $line_of_text));
		
			//echo "$line_of_text";
			$parts = explode(' ', $line_of_text);
			$cmd = $parts[0];
			
			if (in_array($cmd,$commands[$pass]))
			{
				switch ($cmd)
				{
					// Note: fill from 
					case 'fill_from':
						// a vertice
						$v = new Vertice();
						$v->x = (float)$parts[1];
						$v->y = (float)$parts[2];
						$v->z = (float)$parts[3];
						$vertices[] = $v;
						//echo "vertice: ".sizeof($vertices)."\n";
						//echo "  vertice:".print_r($v,1)."\n";
					break;

					case 'v':
						// a vertice
						$v = new Vertice();
						$v->x = (float)$parts[1];
						$v->y = (float)$parts[2];
						$v->z = (float)$parts[3];
						$vertices[] = $v;
						
						//echo "vertice: ".sizeof($vertices)."\n";

						//echo "  vertice:".print_r($v,1)."\n";

					break;

					case 'material':
						// material group...
						$material_name = trim($parts[1]);
						$red = $parts[2];
						$green = $parts[3];
						$blue = $parts[4];
						echoProgress("material definition: {$material_name} $red $green $blue\n");

						if (!in_array($material,$materials))
						{
							$material = new Material();
						}
						else
						{
							$material = $materials[$material_name];
						}
						
						$material->name  = $material_name;
						$material->red   = $red;
						$material->green = $green;
						$material->blue  = $blue;
						$materials[$material_name]=$material;
					break;
					
					case 'usemtl':
						// material group...
						$material = trim($parts[1]);
						echoProgress("usemtl: {$material}\n");
						
						if (!isset($materials[$material]))
						{
							echoProgress("material: {$material} generating random colors...\n");
							$new_material = new Material();
							$new_material->name  = $material;
							$new_material->red   = randomColorValue(65);
							$new_material->green = randomColorValue(65);
							$new_material->blue  = randomColorValue(65);
							$materials[$material]=$new_material;
						}
						$current_material = $materials[$material];
						
					break;
			
					case 'f':
						if ($pass=='v')
						{
							$ftCount++;
						}
						else
						{
							// a face!
							$fCount++;
							$percentComplete = sprintf("%.2f%%",($fCount/$ftCount)*100);
							echoProgress("face processing: $fCount of $ftCount {$percentComplete}");
							
							$verticeCount = sizeof($parts) - 1;
							$faceVertices = Array();
							for ($i = 1; $i <= $verticeCount; $i++)
							{
								$vdata = $parts[$i];
								$vdataParts = explode('/',$vdata);
								$vref = $vdataParts[0]-1; // indexes in files are 1 based
								$actualVertice = $vertices[$vref];
								$faceVertices[] = $actualVertice;
							}

							//echo "face: $fCount material:{$current_material->name}\n";
					
							//echo "  face:".print_r($faceVertices,1)."\n";
							$voxelsOfPolygon = findVoxelsOfPolygon($faceVertices);

							//echo "face: $fCount sizeof(voxelOfPolygon)=".sizeof($voxelsOfPolygon)."\n";
					
							foreach ($voxelsOfPolygon as $voxel)
							{
								$voxel->material_name = $current_material->name;
								add_voxels($voxel);
							}
						}
					break;
			
					default:
						// something we don't currently recognize or care about.
				}
			}
		}
	}
	fclose($inputFile);
	
	// Now that we've made our object. We need to fill it in...
	
	
	if ($outputFileName)
	{
		$outputFile = fopen($outputFileName,'wb');

		$vCount = 0;
		global $all_voxels;
		$vtCount = sizeof($all_voxels);
		foreach ($all_voxels as $voxel)
		{
			$vCount++;

			$percentComplete = sprintf("%.2f%%",($vCount/$vtCount)*100);
			echoProgress("writing file: $vCount of $vtCount {$percentComplete}");
	
		
			$material = $materials[$voxel->material_name];
			$r = $material->red;
			$g = $material->green;
			$b = $material->blue;
			//echo "v:$vCount: starting \n";
			$voxelCode = pointToVoxel($voxel->x,$voxel->y,$voxel->z,$voxel->s,$r,$g,$b);
			//print_r($voxel);
			//echo "v:$vCount: (".sizeof($voxelCode).") ";
			//dumpVoxelCode($voxelCode);
		
			// if we were given an output filename, then write to it.
			if ($outputFile)
			{
				//echo "voxel code size:".sizeof($voxelCode)."\n";
				foreach($voxelCode as $byte)
				{
					$packedByte = pack('C',$byte);
					fwrite($outputFile,$packedByte,1);
				}
			}
		}

		fclose($outputFile);
	}
	
	echo "\n*******\n";
	echo "Total Vertices: ".sizeof($vertices)."\n";
	echo "Total Voxels: ".sizeof($all_voxels)."\n";
	global $maxOctet,$minOctet;
	echo "Shortest Octet length: {$minOctet}\n";
	echo "Largest Octet length: {$maxOctet}\n";
}

// -i inputFileName
// -o outputFileName
// -s minVoxelSize
// -m method (block|wireframe|surface) assumes surface

$options = getopt("i:o:s:m:");

global $renderMode;
//echo "line ".__LINE__." renderMode=$renderMode\n";
// render options
switch($options['m']) {
	case 'blocks':
	case 'wireframe':
	case 'surface':
		$renderMode = $options['m'];
		break;
	default:
		$renderMode = 'surface';
}
//echo "line ".__LINE__." renderMode=$renderMode\n";

// voxelsize
global $minVoxelSize;
//echo "line ".__LINE__." minVoxelSize=$minVoxelSize\n";
if (!empty($options['s'])) {
	if ($options['s'] < 1.0 && $options['s'] > 0.0) {
		$minVoxelSize = $options['s'];
	}
}
//echo "line ".__LINE__." minVoxelSize=$minVoxelSize\n";

//print_r($options);

if (empty($options['i'])) {
	echo "USAGE: objtohio.php -i 'inputFileName' -o 'outputFileName' -m [blocks|wireframe|surface] -s [voxel size] \n";
}
else {
	echo "Convering OBJ file input:{$options['i']} output:{$options['o']} mode:$renderMode voxelSize:{$minVoxelSize}\n";
	convertObjFile($options['i'],$options['o']);
	
	/*
	$filename = $options['i'];
	$file_handle = fopen($filename, "rt"); // these are text files

	$line_of_text = fgets($file_handle);
	echo "read: {$line_of_text}";

	fseek($file_handle,0); // always rewind at the start of each pass
	echo "seek to 0\n";
	
	$line_of_text = fgets($file_handle);
	echo "read: {$line_of_text}";

	fclose($file_handle);
	*/
}
?>
