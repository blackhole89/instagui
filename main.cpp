@include "instagui.h"

int main(int argc, char* argv[])
{
    IGGtk ig;

    int counts[5]={0};
    char counterstr[100] = "0";
    while(1) {
        ig.Start();

        ig.BeginWindow("hello world", 300, 300);

        char buf[10];

        int sum=0;

        for(int i=0;i<5;++i) {
            sprintf(buf,"%d##%d",counts[i],i);
            if(ig.Button(buf)) {
                ++counts[i];
                ig.TimeWarp(); // make label change propagate backwards

                // also increment whatever is in counterstr
                int counterint;
                sscanf(counterstr,"%d",&counterint);
                ++counterint;
                sprintf(counterstr,"%d",counterint);
            }
            sum+=counts[i];
        }

        ig.BeginLine("whatever");

        ig.Text("Counter:");

        sprintf(buf,"%d",sum);
        if(ig.TextInput("flerp",counterstr,9)) {
            printf("New text: %s\n",counterstr);
        }

        ig.EndLine();
    
        ig.EndWindow();
    
        ig.End();

        ig.WaitForStateChange();
    }

    return 0;
}
