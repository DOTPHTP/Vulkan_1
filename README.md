# Vulkan_1
***
create by ShunQi Fan

***
## 1. Introduction
 <b>这是一个用来学习Vulkan api的自用项目，最终的目的是创建一个能够读取指定格式模型并渲染的程序。\
 暂定目标为完成
 普通的模型渲染，不包括动画等功能. \
 主要参考资料为Vulkan-tutorial.com和Vulkan API文档。

 ## 2. Project Structure
 ```

```

***
### 一些说明
#### 1.代码注释
- 由于Vulkan的API过于复杂，很多函数的参数都需要自己去查阅文档，所以在代码中会有很多注释。

#### 2.着色器使用
- shader的编译使用的是glslc工具，编译后的文件会放在shaders目录下。后缀为.spv。也可以在程序
中直接使用glsl的文件，程序会自动编译。

#### 3.库使用

- 使用了GLFW库来创建窗口和处理输入，使用了GLM库来进行数学运算，使用了stb_image库来加载纹理。
- 请使用者自行下载和编译这些库，或者使用vcpkg等工具来管理依赖.


#### 4.分支说明

- main分支为初始分支，仅包含了Vulkan的初始化和窗口的创建等，使用Vulkan官方教程示例。
- debug分支为自主实现的分支，包含obj模型加载和渲染等功能，使用了tinyobjloader库来加载模型。
- 目前分支为3D功能实现分支，采用红蓝立体视觉的方式来实现3D效果。