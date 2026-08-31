import bpy

cam = bpy.context.scene.camera
bpy.context.scene.frame_start = 0
bpy.context.scene.frame_end = 660

# Keyframe 0
bpy.context.scene.frame_set(0)
cam.location = (19.8332, 20.0171, 17.892)
cam.keyframe_insert(data_path='location', index=-1)

# Keyframe 1
bpy.context.scene.frame_set(60)
cam.location = (20.4434, -5.74583, -35.519)
cam.keyframe_insert(data_path='location', index=-1)

# Keyframe 2
bpy.context.scene.frame_set(120)
cam.location = (20.4434, -5.02583, -35.519)
cam.keyframe_insert(data_path='location', index=-1)

# Keyframe 3
bpy.context.scene.frame_set(180)
cam.location = (20.4434, -1.87583, -35.519)
cam.keyframe_insert(data_path='location', index=-1)

# Keyframe 4
bpy.context.scene.frame_set(240)
cam.location = (20.4434, 0.0141745, -35.519)
cam.keyframe_insert(data_path='location', index=-1)

# Keyframe 5
bpy.context.scene.frame_set(300)
cam.location = (27.441, -0.14121, -41.1482)
cam.keyframe_insert(data_path='location', index=-1)

# Keyframe 6
bpy.context.scene.frame_set(360)
cam.location = (34.7854, -0.239645, -49.3122)
cam.keyframe_insert(data_path='location', index=-1)

# Keyframe 7
bpy.context.scene.frame_set(420)
cam.location = (44.1329, -0.4222, -59.7027)
cam.keyframe_insert(data_path='location', index=-1)

# Keyframe 8
bpy.context.scene.frame_set(480)
cam.location = (44.1329, 0.387801, -59.7027)
cam.keyframe_insert(data_path='location', index=-1)

# Keyframe 9
bpy.context.scene.frame_set(540)
cam.location = (54.8158, 0.179162, -71.5777)
cam.keyframe_insert(data_path='location', index=-1)

# Keyframe 10
bpy.context.scene.frame_set(600)
cam.location = (58.1542, 0.788964, -75.2886)
cam.keyframe_insert(data_path='location', index=-1)

