


var wallX = 700;
var wallY = 700;
var wallZ = 700;//location

var VOXELSIZE=8;
var red=225;
var blue=0;
var green=0;//color brightness
var currentStep=0;//counting number of steps
var direction=1;//left to right color change
var height=8;
var width=8;


var currentStep=0;




function step()
{
    
    
    
    currentStep++;
    
    if(currentStep>6000)//how long it will run
        Script.stop();
    
    
    for(var i=0;i<width;i++)
    {
        for(var j=0;j<height;j++)
        {
            
            Voxels.setVoxel(wallX+i*VOXELSIZE, wallY+j*VOXELSIZE, wallZ, VOXELSIZE, red,green,blue);
            
            
            
        }
        
    }

    if (direction==1)
    {
        if(blue<255)
        {
            blue++;
            red--;
          
        }
        else
        {
            direction=0;
        }
        
    }
    else if(direction==0)
    {
        if(blue>0)
        {
            blue--;
            red++;
        
        }
        else
        {
            direction=1;
        }
    }
    
   
   
    
    
    
    
}




Script.update.connect(step);
Voxels.setPacketsPerSecond(20000);
















