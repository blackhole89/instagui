#include <string>
#include <vector>
#include <thread>
#include <stack>
#include <map>
#include <mutex>
#include <condition_variable>
#include <functional>

typedef std::size_t ident_t;

@include "algtypes.h"

namespace IGImpl {

datatype IGVDOM;

struct IGVDOMBase {
    ident_t id;

    bool dirty;

    std::vector<IGVDOM> children;
};

datatype IGVDOM : public IGVDOMBase
 =   Root
   | Window(std::string, int, int)
   | VBox
   | HBox
   | Button(std::string)
   | TextInput(std::string)
   | Text(std::string);

struct IGStateBase {
    ident_t id;

    bool view2model, model2view;
};

datatype IGState : public IGStateBase
 =   TrivialState
   | WindowState(int,int,bool)
   | TextState(std::string)
   | ButtonState(bool);

}

class IG {
protected:
    /* vdom root */
    std::vector<IGImpl::IGVDOM> root;

    /* mutable state */
    std::map<ident_t, IGImpl::IGState> state;

    /* stacks */
    std::vector<std::vector<IGImpl::IGVDOM>*> con_stack;
    std::vector<std::vector<IGImpl::IGVDOM>::iterator> pos;

    std::vector<IGImpl::IGVDOM> dom_stack;
    std::vector<ident_t> id_stack;

    /* queue of DOM elements to remove */
    std::vector<IGImpl::IGVDOM> to_unrealise;
    
    /* state updates to propagate */
    std::mutex updown_mutex;
    std::condition_variable updates_present; 
    std::vector<IGImpl::IGState> updates;

    virtual void Realise(IGImpl::IGVDOM ent, IGImpl::IGVDOM parent) =0;
    virtual void Unrealise(IGImpl::IGVDOM ent) =0;

    void ViewToModelSt(ident_t id, IGImpl::IGState nstate);
    virtual void ModelToViewSt(IGImpl::IGVDOM ent) =0;

    virtual void RunInGUIThread(std::function<void ()> fn) =0;

    void RealiseIfDirty(IGImpl::IGVDOM d, IGImpl::IGVDOM parent);

    ident_t GetIdent(std::string text);
    std::pair<ident_t,std::string> SplitIdent(std::string s);

    void MaybeUpdateState(ident_t id, IGImpl::IGState nstate);
    IGImpl::IGVDOM MaybeAttach(ident_t id, IGImpl::IGVDOM node);
    
    void DOMEnter();
    void DOMLeave();
public:
    bool BeginWindow(std::string title, int sx, int sy);
    void EndWindow();

    bool Button(std::string label);

    void TimeWarp();

    virtual void Start();
    virtual void End();

    void WaitForStateChange();

    IG();
    virtual ~IG();
};


struct IGGtkState;

class IGGtk : public IG {
protected:
    virtual void Realise(IGImpl::IGVDOM ent, IGImpl::IGVDOM parent);
    virtual void Unrealise(IGImpl::IGVDOM ent);

    virtual void ModelToViewSt(IGImpl::IGVDOM ent);

    virtual void RunInGUIThread(std::function<void ()> fn);   

    std::thread guithread;

    std::mutex cl_mutex;
    std::condition_variable cv_cl_done; 
    bool cl_done;
    std::function<void ()> cl;
    void RunClosure();

    IGGtkState *st;
public:
    IGGtk();
    
//    virtual void Start();
//    virtual void End();

    virtual ~IGGtk();
};

