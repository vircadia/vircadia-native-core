


var wallX = 8000;
var wallY = 8000;
var wallZ = 8000;//location


var VOXELSIZE=12;//size of each voxel

var FACTOR = 0.75;

var loud=1.0;
var maxLoud=500;
var minLoud=200;//range of loudness



var maxB={color:225,direction:0,speed:1};
var minB={color:50,direction:1,speed:1};
var maxG={color:200,direction:0,speed:1};
var minG={color:30,direction:1,speed:1};
var maxR={color:255,direction:0,speed:1};
var minR={color:150,direction:1,speed:1};//color objects
var addVoxArray=[];
var removeVoxArray=[];
var numAddVox=0;
var numRemoveVox=0;//array for voxels removed and added


var height;
var wallWidth=34;
var wallHeight=25;
var maxHeight=wallHeight;
var minHeight=0;//properties of wall


var heightSamplesArray=[];
var sampleIndex=0;//declare new array of heights

var direction=1;




//initiate and fill array of heights
for(var k=0;k<wallWidth;k++)
{
    heightSamplesArray[k]=0;
}


//send objects to function changeColor
function scratch()
{
    
    
    changeColor(maxB);
    changeColor(minB);
    changeColor(maxG);
    changeColor(minG);
    changeColor(maxR);
    changeColor(minR);
    
    //calculates loudness
    var audioAverageLoudness = MyAvatar.audioAverageLoudness * FACTOR;
    
    
    loud = Math.log(audioAverageLoudness) / 5.0 * 255.0;
    
    print("loud="+ loud);
    
    
    var scalingfactor=(loud-minLoud)/(maxLoud-minLoud);
    if(scalingfactor<0)
    {
        scalingfactor=0;
    }
    if(scalingfactor>1)
    {
        scalingfactor=1;
    }
    
    //creates diff shades for diff levels of volume
    
    var green=(maxG.color-minG.color)*scalingfactor+minG.color;
    var blue=(maxB.color-minB.color)*scalingfactor+minB.color;
    var red=(maxR.color-minR.color)*scalingfactor+minR.color;
    height=(maxHeight-minHeight)*scalingfactor+minHeight;
    
    
    //sets height at position sampleIndex
    heightSamplesArray[sampleIndex]=height;
  
        
    
    
    if(loud==Number.NEGATIVE_INFINITY)
    {
        green=minG.color;
        blue=minB.color;
        red=minR.color;
        
    }
    
    
    var k=sampleIndex;
    
    //add&remove voxels
    
    for(var i=wallWidth-1;i>=0;i--)
    {
        
        for(var j=0;j<wallHeight;j++)
            
        {
            if(j<=heightSamplesArray[k])
            {
                addVoxArray[numAddVox]={x:wallX+i*VOXELSIZE, y:wallY+j*VOXELSIZE, z:wallZ};
            
                    numAddVox++;
                
            }
            else
            {
                removeVoxArray[numRemoveVox]={x:wallX+i*VOXELSIZE, y:wallY+j*VOXELSIZE, z:wallZ};
                
                numRemoveVox++;
            }
            
        }
        k--;
        if(k<0)
        {
            k=wallWidth-1;
        }
        
    }
    
    for(var k=0;k<numAddVox;k++)
    {
        Voxels.setVoxel(addVoxArray[k].x,addVoxArray[k].y,addVoxArray[k].z,VOXELSIZE, red, green, blue);
    }
    
    for(var k=0;k<numRemoveVox;k++)
    {
        Voxels.eraseVoxel(removeVoxArray[k].x,removeVoxArray[k].y,removeVoxArray[k].z,VOXELSIZE);
    }
    
    numAddVox=0;
    numRemoveVox=0;
    
    sampleIndex++;
    if(sampleIndex>=wallWidth)
    {
        sampleIndex=0;
    }
    
    
    
}

//color properties (shade, direction, speed)

function changeColor(color)
{

    if (color.direction==1)
    {
        if(color.color<255)
        {
            color.color+=(color.speed);
            
        
        }
        else
        {
            color.direction=0;
        }
    
    }
    else if(color.direction==0)
    {
        if(color.color>0)
        {
            color.color-=(color.speed);
            
        
        }
        else
        {
            color.direction=1;
        }
    }
    
}



























Script.update.connect(scratch);
Voxels.setPacketsPerSecond(20000);

