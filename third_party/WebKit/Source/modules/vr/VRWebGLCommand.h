#ifndef VRWebGLCommand_h
#define VRWebGLCommand_h

// #define VRWEBGL_SHOW_LOG

#define VRWEBGL_USE_CACHE

// TODO: For some reason GLES3.0 does not define this extension macro... so we add it manually! Used in video handling.
#define GL_TEXTURE_EXTERNAL_OES           0x8D65

#include <string>

class VRWebGLCommand
{
private:
    bool m_insideAFrame = false;

    // Do not allow copy of instances.
    VRWebGLCommand(const VRWebGLCommand&) = delete;
    VRWebGLCommand& operator=(const VRWebGLCommand&) = delete;
    
protected:
    // Do not allow to create instances from outside of this class and the classes that inherit from it.
    VRWebGLCommand();

public:
    
    virtual ~VRWebGLCommand();
    
    void setInsideAFrame(bool insideAFrame);
    bool insideAFrame() const;

    virtual bool isForUpdate() const;

    virtual bool isSynchronous() const = 0;

    virtual bool canBeProcessedImmediately() const = 0;
    
    virtual void* process() = 0;
    
    virtual std::string name() const = 0;

	static unsigned int numInstances;    
};

class VRWebGLCommandForUpdate: public VRWebGLCommand
{
};


#endif // VRWebGLCommand_h