@include "instagui.h"

int main(int argc, char* argv[])
{
    IGGtk ig;

    int counts[5]={0};
    while(1) {
        ig.Start();

        ig.BeginWindow("hello world", 300, 300);

        char buf[10];

        for(int i=0;i<5;++i) {
            sprintf(buf,"%d##%d",counts[i],i);
            if(ig.Button(buf)) {
                ++counts[i];
                ig.TimeWarp(); // make label change propagate backwards
            }
        }
    
        ig.EndWindow();
    
        ig.End();

        ig.WaitForStateChange();
    }

    return 0;
}
