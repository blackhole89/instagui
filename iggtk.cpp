#include <gtkmm.h>

@include "instagui.h"

using namespace IGImpl;

struct IGGtkState {
    IGGtk *ig;

    std::thread guithread;

    Glib::RefPtr<Gtk::Application> app;
    Glib::Dispatcher d;

    Glib::Dispatcher quit;

    std::map<ident_t, Gtk::Widget* > widgets;

    void Quit() { Gtk::Main::quit(); }

    IGGtkState() {}
};

IGGtk::IGGtk()
{
    cl_done=false;

    guithread = std::thread([this]() {
        st = new IGGtkState();
        st->ig = this;
        st->app = Gtk::Application::create("arpa.instagui");
        st->d.connect(sigc::mem_fun(this, &IGGtk::RunClosure));
        st->quit.connect(sigc::mem_fun(*st, &IGGtkState::Quit));

        st->app->signal_startup().connect([this](){
            printf("initialised.\n");
            auto w=new Gtk::Window(); //w->show();
            st->app->add_window(*w);
            std::lock_guard<std::mutex> lk(cl_mutex);
            cl_done=true;
            cv_cl_done.notify_all();
        });

        st->app->run();
    });

    std::unique_lock<std::mutex> lk(cl_mutex);
    cv_cl_done.wait(lk, [this]{ return cl_done; });
}

IGGtk::~IGGtk()
{
    st->quit.emit();
}

void IGGtk::RunClosure()
{
    cl();

    std::lock_guard<std::mutex> lk(cl_mutex);
    cl_done=true;
    cv_cl_done.notify_all();
}

void IGGtk::RunInGUIThread(std::function<void ()> fn)
{
    cl=fn;
    cl_done=false;

    st->d.emit();

    std::unique_lock<std::mutex> lk(cl_mutex);
    cv_cl_done.wait(lk, [this]{ return cl_done; });
}

void IGGtk::Realise(IGVDOM ent, IGVDOM parent)
{
    ent->dirty=false;
    printf("realise ID %zX\n", ent->id);

    if(!st->widgets.count(ent->id)) {

        Gtk::Widget *widget;
        match(ent) {
        case Window(&title,&sx,&sy):
            Gtk::Window *wnd = new Gtk::Window();
    //Gtk::make_managed<Gtk::Window>();
            wnd->set_title(title);
            wnd->set_size_request(sx,sy);
            st->app->add_window(*wnd);
            widget=wnd;
        case VBox:
            widget = Gtk::make_managed<Gtk::VBox>();
        case HBox:
            widget = Gtk::make_managed<Gtk::HBox>();
        case Button(&label):
            Gtk::Button *btn = Gtk::make_managed<Gtk::Button>(label);
            ident_t entid = ent->id;
            btn->signal_clicked().connect( [this, entid]() {
                ViewToModelSt(entid, ButtonState(true));
            });
            widget = btn;
        default:
        }

        st->widgets[ent->id]=widget;

        match(parent) {
        case Root:
        case VBox:
            Gtk::VBox *parent_ctr = static_cast<Gtk::VBox*>(st->widgets[parent->id]);       
            parent_ctr->pack_start(*widget);
        case HBox:
            Gtk::HBox *parent_ctr = static_cast<Gtk::HBox*>(st->widgets[parent->id]);       
            parent_ctr->pack_start(*widget);
        default:
            Gtk::Bin *parent_ctr = static_cast<Gtk::Bin*>(st->widgets[parent->id]);       
            parent_ctr->add(*widget);      
        }

        widget->show_all();
    }

    for(auto &d : ent->children) {
        Realise(d, ent);
    }
}

void IGGtk::Unrealise(IGVDOM ent)
{
    printf("unrealise ID %zX\n", ent->id);
    for(auto &d : ent->children) {
        Unrealise(d);
    }
    if(st->widgets.count(ent->id)) {
        delete st->widgets[ent->id];
        st->widgets.erase(ent->id);
    }
}

void IGGtk::ModelToViewSt(IGVDOM ent)
{
}

