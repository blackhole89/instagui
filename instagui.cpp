@include "instagui.h"

#include <string.h>

using namespace IGImpl;

ident_t IG::GetIdent(std::string s)
{
    ident_t ret=0;
    for(ident_t i : id_stack) {
        ret=(ret<<1)^i;
    } 
    return (ret<<1)^std::hash<std::string>{}(s);
}

std::pair<ident_t,std::string> IG::SplitIdent(std::string s)
{
    return std::make_pair<ident_t,std::string>(GetIdent(s), s.substr(0,s.find("##")));
}

IG::IG()
{
}

IG::~IG()
{
}

void IG::ViewToModelSt(ident_t id, IGState st)
{
    std::lock_guard<std::mutex> lk(updown_mutex);
    st->id=id;
    updates.push_back(st);
    updates_present.notify_all();
}

void IG::TimeWarp()
{
    std::lock_guard<std::mutex> lk(updown_mutex);
    updates.push_back(TrivialState); 
    updates_present.notify_all();
}

void IG::WaitForStateChange()
{
    std::unique_lock<std::mutex> lk(updown_mutex);
    updates_present.wait(lk, [this]{ return updates.size()>0; });
}

void IG::Start()
{
    /* download updates */
    updown_mutex.lock();
    for(auto &u : updates) {
        u->view2model=true;
        if(u->id) state[u->id] = u;
    } 
    updates.clear();
    updown_mutex.unlock();

    pos.clear();
    pos.push_back(root.begin());
    con_stack.push_back(&root);
    dom_stack.clear();
    id_stack.clear();
}

void IG::RealiseIfDirty(IGImpl::IGVDOM d, IGImpl::IGVDOM parent)
{
    if(d->dirty) {
        Realise(d, parent);
    } else {
        for(auto &dnext:d->children) {
            RealiseIfDirty(dnext, d);
        }
    }
}

void IG::End()
{
    RunInGUIThread([this]() {
        for(auto &d : to_unrealise) {
            Unrealise(d);
        }
        to_unrealise.clear();
        for(auto &e : root) {
            RealiseIfDirty(e, Root);
        }
    });
}

/* widgets */
IGVDOM IG::MaybeAttach(ident_t id, IGVDOM node)
{
    if(pos.back() != con_stack.back()->end()) {
        if((*pos.back())->id == id) {
            return *pos.back();
        } else {
            printf("ID mismatch: %zX, %zX\n",id, (*pos.back())->id);
            to_unrealise.insert(to_unrealise.end(), pos.back(), con_stack.back()->end());
            con_stack.back()->erase(pos.back(),con_stack.back()->end());
        }
    }
    node->id = id;
    con_stack.back()->push_back(node);
    pos.pop_back(); pos.push_back(--con_stack.back()->end());
    if(dom_stack.size()) {
        dom_stack.back()->dirty=true;
    } else {
        node->dirty=true;
    }
    return node;

}

void IG::DOMEnter()
{
    if(pos.back() != con_stack.back()->end()) {
        IGVDOM d = *pos.back();
        id_stack.push_back(d->id);
        dom_stack.push_back(d);
        con_stack.push_back(&d->children);
        pos.push_back(d->children.begin());
    }   
}

void IG::DOMLeave()
{
    id_stack.pop_back();
    dom_stack.pop_back();
    con_stack.pop_back();
    pos.pop_back();
}

void IG::MaybeUpdateState(ident_t id, IGState nstate)
{
    if(state.count(id)) {
    }
}

bool IG::BeginWindow(std::string title, int sx, int sy)
{
    ident_t id = GetIdent(title);

    if(state.count(id)) {
        match(state[id]) {
        case WindowState(&sx2,&sy2,&open):
            if(!open) return false;
        }
    } else state[id] = WindowState(sx,sy,true);

    IGVDOM d = MaybeAttach(id, Window(title, sx, sy) ); 

    DOMEnter();

    id = GetIdent(title);

    d = MaybeAttach(id, VBox);

    DOMEnter();

    return true;
}

void IG::EndWindow()
{
    DOMLeave();
    DOMLeave();
    ++pos.back();
}

void IG::BeginLine(std::string name)
{
    ident_t id = GetIdent(name);
    
    MaybeAttach(id, HBox);
    
    DOMEnter();
}

void IG::EndLine()
{
    DOMLeave();
    ++pos.back();
}

bool IG::Button(std::string label)
{
    auto [id,text] = SplitIdent(label);

    MaybeAttach(id, ::Button(text));

    ++pos.back();

    if(state.count(id)) {
        match(state[id]) {
        case ButtonState(&pressed):
            state[id]=ButtonState(false);
            return pressed;
        default:
        }
    } else state[id]=ButtonState(false);
    return false;
}

void IG::Text(std::string label)
{
    auto [id,text] = SplitIdent(label);

    MaybeAttach(id, ::Text(text));

    ++pos.back();
}

bool IG::TextInput(std::string name, char *buf, int buf_length)
{
    auto [id,dummy] = SplitIdent(name);

    auto node = MaybeAttach(id, ::TextInput);

    ++pos.back();

    if(state.count(id)) {
        match(state[id]) {
        case TextState(&text,&commit):
            if(!state[id]->view2model) {
                state[id]=TextState(std::string(buf),false);
                printf("invalidate %zX\n",id);
                ModelToViewSt(node);
                return false;
            } else {
                printf("download %zX\n",id);
                state[id]=TextState(text,false);
                printf("state.id.view2model is %d\n",state[id]->view2model);
                strncpy(buf, text.c_str(), buf_length);
                return commit; //or return true?
            }
        default:
        }
    } else state[id]=TextState(std::string(buf),false);
    return false;
}

