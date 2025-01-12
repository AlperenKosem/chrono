#------------------------------------------------------------------------------
# Name:        pychrono example
# Purpose:
#
# Author:      Alessandro Tasora
#
# Created:     1/01/2019
# Copyright:   (c) ProjectChrono 2019
#------------------------------------------------------------------------------

print ("Example: Load a STEP file, generated by a CAD.");
print ("Please wait! this may take a while to load the file..."); 

import pychrono.core as chrono
import pychrono.irrlicht as chronoirr
import pychrono.cascade as cascade
from OCC.Core import TopoDS

    
# The path to the Chrono data directory containing various assets (meshes, textures, data files)
# is automatically set, relative to the default location of this demo.
# If running from a different directory, you must change the path to the data directory with: 
#chrono.SetChronoDataPath('path/to/data')


# ---------------------------------------------------------------------
#
#  Create the simulation system and add items
#

mysystem      = chrono.ChSystemNSC()

# Load a STEP file, containing a mechanism. The demo STEP file has been
# created using a 3D CAD (in this case, SolidEdge v.18).
#

# Create the ChCascadeDoc, a container that loads the STEP model
# and manages its subassembles
mydoc = cascade.ChCascadeDoc()


# load the STEP model using this command:
load_ok = mydoc.Load_STEP(chrono.GetChronoDataFile('cascade/IRB7600_23_500_m2000_rev1_01_decorated.stp'))  # or specify abs.path: ("C:\\data\\cascade\\assembly.stp");

if not load_ok:
    raise ValueError("Warning. Desired STEP file could not be opened/parsed \n")
      


CH_C_PI = 3.1456

# In most CADs the Y axis is horizontal, but we want it vertical.
# So define a root transformation for rotating all the imported objects.
rotation1 = chrono.ChQuaternionD()
rotation1.Q_from_AngAxis(-CH_C_PI / 2, chrono.ChVectorD(1, 0, 0))  # 1: rotate 90° on X axis
rotation2 = chrono.ChQuaternionD()
rotation2.Q_from_AngAxis(CH_C_PI, chrono.ChVectorD(0, 1, 0))  # 2: rotate 180° on vertical Y axis
tot_rotation = chrono.ChQuaternionD()
tot_rotation = rotation2 % rotation1     # rotate on 1 then on 2, using quaternion product
root_frame = chrono.ChFrameMovingD(chrono.ChVectorD(0, 0, 0), tot_rotation)

    
# Retrieve some sub shapes from the loaded model, using
# the GetNamedShape() function, that can use path/subpath/subsubpath/part
# syntax and * or ? wildcards, etc.

def make_body_from_name(partname, root_transformation):
    shape1 = TopoDS.TopoDS_Shape()
    if (mydoc.GetNamedShape(shape1, partname)):
        # Make a ChBody representing the TopoDS_Shape part from the CAD:
        mbody1 = cascade.ChBodyEasyCascade(shape1, # shape
                                           1000,   # density (center of mass & inertia automatically computed)
                                           True,    # mesh for visualization?
                                           False)   # mesh for collision?
        mysystem.Add(mbody1)
        # Move the body as for global displacement/rotation (also mbody1 %= root_frame; )
        mbody1.ConcatenatePreTransformation(root_transformation)
        return mbody1
    else:
        raise ValueError("Warning. Body part name cannot be found in STEP file.\n")
      
      
mrigidBody_base     = make_body_from_name("Assem10/Assem8", root_frame)
mrigidBody_turret   = make_body_from_name("Assem10/Assem4", root_frame)
mrigidBody_bicep    = make_body_from_name("Assem10/Assem1", root_frame)
mrigidBody_elbow    = make_body_from_name("Assem10/Assem5", root_frame)
mrigidBody_forearm  = make_body_from_name("Assem10/Assem7", root_frame)
mrigidBody_wrist    = make_body_from_name("Assem10/Assem6", root_frame)
mrigidBody_hand     = make_body_from_name("Assem10/Assem9", root_frame)
mrigidBody_cylinder = make_body_from_name("Assem10/Assem3", root_frame)
mrigidBody_rod      = make_body_from_name("Assem10/Assem2", root_frame)

mrigidBody_base.SetBodyFixed(True)
#mrigidBody_hand.SetBodyFixed(True)

# Create joints between two parts.
# To understand where is the axis of the joint, we can exploit the fact
# that in the STEP file that we prepared for this demo, we inserted some
# objects called 'marker' and we placed them aligned to the shafts, so now
# we can fetch them and get their position/rotation.

def make_frame_from_name(partname, root_transformation):
    shape_marker = TopoDS.TopoDS_Shape()
    if (mydoc.GetNamedShape(shape_marker, partname)):
        frame_marker = chrono.ChFrameD()
        mydoc.FromCascadeToChrono(shape_marker.Location(), frame_marker)
        frame_marker.ConcatenatePreTransformation(root_transformation)
        return frame_marker
    else:
        raise ValueError("Warning. Marker part name cannot be found in STEP file.\n")

frame_marker_base_turret    = make_frame_from_name("Assem10/Assem8/marker#1", root_frame)
frame_marker_turret_bicep   = make_frame_from_name("Assem10/Assem4/marker#2", root_frame)
frame_marker_bicep_elbow    = make_frame_from_name("Assem10/Assem1/marker#2", root_frame)
frame_marker_elbow_forearm  = make_frame_from_name("Assem10/Assem5/marker#2", root_frame)
frame_marker_forearm_wrist  = make_frame_from_name("Assem10/Assem7/marker#2", root_frame)
frame_marker_wrist_hand     = make_frame_from_name("Assem10/Assem6/marker#2", root_frame)
frame_marker_turret_cylinder= make_frame_from_name("Assem10/Assem4/marker#3", root_frame)
frame_marker_cylinder_rod   = make_frame_from_name("Assem10/Assem3/marker#2", root_frame)
frame_marker_rod_bicep      = make_frame_from_name("Assem10/Assem2/marker#2", root_frame)

                                                   
# Create joints between the parts. 
# This can be done by creating link objects between couples of the bodies
# created in the section above, where the joint position is one of the 
# frame_marker_xxxx_zzzz frames created above.

my_link1 = chrono.ChLinkLockRevolute()
my_link1.Initialize(mrigidBody_base, mrigidBody_turret, frame_marker_base_turret.GetCoord())
mysystem.Add(my_link1)
    
my_link2 = chrono.ChLinkLockRevolute()                                               
my_link2.Initialize(mrigidBody_turret, mrigidBody_bicep, frame_marker_turret_bicep.GetCoord())
mysystem.Add(my_link2)  
        
my_link3 = chrono.ChLinkLockRevolute()
my_link3.Initialize(mrigidBody_bicep, mrigidBody_elbow, frame_marker_bicep_elbow.GetCoord())
mysystem.Add(my_link3)

my_link4 = chrono.ChLinkLockRevolute()
my_link4.Initialize(mrigidBody_elbow, mrigidBody_forearm, frame_marker_elbow_forearm.GetCoord())
mysystem.Add(my_link4)

my_link5 = chrono.ChLinkLockRevolute()
my_link5.Initialize(mrigidBody_forearm, mrigidBody_wrist, frame_marker_forearm_wrist.GetCoord())
mysystem.Add(my_link5)

my_link6 = chrono.ChLinkLockRevolute()
my_link6.Initialize(mrigidBody_wrist, mrigidBody_hand, frame_marker_wrist_hand.GetCoord())
mysystem.Add(my_link6)

my_link7 = chrono.ChLinkLockRevolute()
my_link7.Initialize(mrigidBody_turret, mrigidBody_cylinder, frame_marker_turret_cylinder.GetCoord())
mysystem.Add(my_link7)

my_link8 = chrono.ChLinkLockRevolute()
my_link8.Initialize(mrigidBody_cylinder, mrigidBody_rod, frame_marker_cylinder_rod.GetCoord())
mysystem.Add(my_link8)
        
my_link9 = chrono.ChLinkLockRevolute()
my_link9.Initialize(mrigidBody_rod, mrigidBody_bicep, frame_marker_rod_bicep.GetCoord())
mysystem.Add(my_link9)

                         
# Create a large cube as a floor.

mfloor = chrono.ChBodyEasyBox(5, 1, 5, 1000, True, True)
mfloor.SetPos(chrono.ChVectorD(0,-0.5,0))
mfloor.SetBodyFixed(True)
mysystem.Add(mfloor)

mcolor = chrono.ChColorAsset(0.3, 0.3, 0.8)
mfloor.AddAsset(mcolor)


# We want to move the hand of the robot using a trajectory.
# Since in this demo all joints are 'off' (i.e just revolute joints),
# it follows that if we move the hand all the robot will automatically
# move as in inverse kinematics.

# Create a ChLinePath geometry, for the hand path, and insert arc/lines sub-paths:
mpath = chrono.ChLinePath()
ma1 = chrono.ChLineArc(
            chrono.ChCoordsysD(mrigidBody_hand.GetPos(), # arc center position
                               chrono.Q_ROTATE_X_TO_Z),   # arc plane alignment (default: xy plane) 
            0.3, # radius 
            -chrono.CH_C_PI_2, # start arc ngle (counterclockwise, from local x)
            -chrono.CH_C_PI_2+chrono.CH_C_2PI, # end arc angle 
            True)
mpath.AddSubLine(ma1)
mpath.SetPathDuration(2)
mpath.Set_closed(True)

# Create a ChLineShape, a visualization asset for lines.
mpathasset = chrono.ChLineShape()
mpathasset.SetLineGeometry(mpath)
mfloor.AddAsset(mpathasset)

# This is the constraint that uses the trajectory
mtrajectory = chrono.ChLinkTrajectory()
# Define which parts are connected (the trajectory is considered in the 2nd body).
mtrajectory.Initialize(mrigidBody_hand, # body1 that follows the trajectory
          mfloor,                 # body2 that 'owns' the trajectory
          chrono.VNULL,           # point on body1 that will follow the trajectory, in body1 coords
          mpath                   # the trajectory (reuse the one already added to body2 as asset)
          )
mysystem.Add(mtrajectory)
# Optionally, set a function that gets the curvilinear
# abscyssa s of the line, as a function of time s(t). 
# By default it was simply  s=t.
mspacefx = chrono.ChFunction_Ramp(0, 0.5)
mtrajectory.Set_space_fx(mspacefx)

# Just to constraint the hand rotation:
mparallelism = chrono.ChLinkLockParallel()
mparallelism.Initialize(mrigidBody_hand, mfloor, frame_marker_wrist_hand.GetCoord())
mysystem.Add(mparallelism);

# ---------------------------------------------------------------------
#
#  Create an Irrlicht application to visualize the system
#

myapplication = chronoirr.ChIrrApp(mysystem, 'Import STEP', chronoirr.dimension2du(1024,768))

myapplication.AddSkyBox()
myapplication.AddLogo(chrono.GetChronoDataFile('logo_pychrono_alpha.png'))
myapplication.AddCamera(chronoirr.vector3df(2,2,2),chronoirr.vector3df(0,0.8,0))
#myapplication.AddTypicalLights()
myapplication.AddLightWithShadow(chronoirr.vector3df(3,6,2),    # point
                                 chronoirr.vector3df(0,0,0),    # aimpoint
                                 12,                 # radius (power)
                                 1,11,              # near, far
                                 55)                # angle of FOV

            # ==IMPORTANT!== Use this function for adding a ChIrrNodeAsset to all items
			# in the system. These ChIrrNodeAsset assets are 'proxies' to the Irrlicht meshes.
			# If you need a finer control on which item really needs a visualization proxy in
			# Irrlicht, just use application.AssetBind(myitem); on a per-item basis.

myapplication.AssetBindAll();

			# ==IMPORTANT!== Use this function for 'converting' into Irrlicht meshes the assets
			# that you added to the bodies into 3D shapes, they can be visualized by Irrlicht!

myapplication.AssetUpdateAll();

            # If you want to show shadows because you used "AddLightWithShadow()'
            # you must remember this:
myapplication.AddShadowAll();

# ---------------------------------------------------------------------
#
#  Run the simulation
#

# Change the solver form the default SOR to a more precise solver

#msolver = mkl.ChSolverMKLcsm()
#mysystem.SetSolver(msolver)

mysystem.SetSolverType(chrono.ChSolver.Type_BARZILAIBORWEIN);
#mysystem.SetSolverType(chrono.ChSolver.Type_MINRES);
mysystem.SetSolverMaxIterations(300)

myapplication.SetTimestep(0.01)


while(myapplication.GetDevice().run()):
    myapplication.BeginScene()
    myapplication.DrawAll()
    myapplication.DoStep()
    myapplication.EndScene()





