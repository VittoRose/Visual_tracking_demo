#include    "RealTimeTask.h""

#define     YWIN        720         // Screen dimensions
#define     XWIN        1080
#define     XRBOUND     1065
#define     YUBOUND     65
#define     XLBOUND     11
#define     YLBOUND     705

#define     BLACK   0           // Allegro color
#define     BLUE    1
#define     GREEN   2
#define     CYAN    3
#define     RED     4
#define     LCYAN   11
#define     WHITE   15
#define     BKG     7

#define     WIDTH   140         // Button dimensions
#define     TALL    30
#define     PITCH   10
#define     SIDE    20
#define     BPITCH  40

#define     CITY        10          // background options
#define     FOREST      20
#define     DESERT      30
#define     DEFAULT     40

#define     OPTION      1           // menu section
#define     COMMAND     2
#define     RUN         3
#define     ABORT       4

#define     BUFFER     11           //button draw on buffer or screen
#define     SCREEN     21

#define     SIN        1            // type of target motion
#define     LIN        2
#define     MANUAL     3 

#define     N            0             //scanning costant
#define     Y            1 
#define     THRESHOLD    300

int find = N;
int prediction = Y;
int path = SIN;
int multiplier = 1;
int background = DEFAULT;
int end = 0;
int amplitude = 10;
int color = 1;
float C = 4;

sem_t x_sem, y_sem, obj_sem, find_sem, c_sem, color_sem, motion_sem;

double velocity_x = 0.02;
double slope = 1;

BITMAP *bkground;
BITMAP *buffer;

struct camera_window{
    int x;
    int y;
    int w;
    int h;
}cam;

struct deadline{
    int motion;
    int draw;
    int scan;
    int interaction;
    int xmotor;
    int ymotor;
}dl;

struct point{
    int x;
    int y;
}obj, ref;


void init(){
    // start allegro library
    
    allegro_init();
    set_gfx_mode(GFX_AUTODETECT_WINDOWED,XWIN,YWIN,0,0);
    set_color_depth(8);
    clear_to_color(screen,BKG);
    install_keyboard();
    install_mouse();
    enable_hardware_cursor();
    show_mouse(screen);

    ptask_init(SCHED_FIFO);

}

void draw_hud(){
    // draw the hud displayed in the run section
    char v[35], a[20], c[26], d[15], e[15], f[15], g[15], h[15], i[15];

    line(buffer, 0, 2*PITCH + TALL, XWIN, 2*PITCH + TALL, BLACK);
    line(buffer, 0, 2*PITCH + TALL + 1, XWIN, 2*PITCH + TALL + 1, BLACK);

    if(path == LIN){
        rectfill(buffer, WIDTH + 2*PITCH, 2*PITCH + TALL - 1, XWIN, 0, BKG);
        sprintf(v, "VELOCITY = %1.4f pixel/period", velocity_x);
        sprintf(a, "Angle: %3.0f", slope);
        textout_centre_ex(buffer, font, v,  2*WIDTH, PITCH, BLACK, -1);
    }

    if(path == SIN){
        rectfill(buffer, WIDTH + 2*PITCH, 2*PITCH + TALL - 1, XWIN, 0, BKG);
        sprintf( v, "VELOCITY = %1.4f pixel/period", velocity_x);
        sprintf( a, "AMPLITUDE = %d pixel", amplitude);
        textout_centre_ex(buffer, font, v,  2*WIDTH, PITCH, BLACK, -1);
        textout_centre_ex(buffer, font, a, 2*WIDTH, 3*PITCH, BLACK, -1);
    }

    if(path == MANUAL){
        rectfill(buffer, WIDTH + 2*PITCH, 2*PITCH + TALL - 1, XWIN, 0, BKG);
        sprintf(v, "VELOCITY = %d pixel/period", multiplier*2);
        textout_centre_ex(buffer, font, v,  2*WIDTH, PITCH, BLACK, -1);
    }

    sem_wait(&c_sem);
    sprintf(c, "CONTROLLER CONSTANT: %2.2f", C);
    sem_post(&c_sem);

    textout_centre_ex(buffer, font, c, XWIN/2, PITCH, BLACK, -1);
    if(prediction == Y){
        textout_centre_ex(buffer, font, "PREDICTION: ON", XWIN/2, 2*PITCH, BLACK, -1);
    }else {
        textout_centre_ex(buffer, font, "PREDICTION: OFF", XWIN/2, 2*PITCH, BLACK, -1);
    }

    textout_centre_ex(buffer, font, "DEADLINE MISSED: ", XWIN*2/3 + 40, PITCH, BLACK, -1);

    sprintf(d, "MOTOR X %d", dl.xmotor);
    sprintf(e, "MOTOR Y %d", dl.ymotor);
    sprintf(f, "CAMERA  %d", dl.scan);
    sprintf(g, "USER    %d", dl.interaction);
    sprintf(h, "MOTION  %d", dl.motion);
    sprintf(i, "DRAW    %d", dl.draw);

    textout_centre_ex(buffer, font, d, XWIN*2/3 + 160, PITCH, BLACK, -1);
    textout_centre_ex(buffer, font, e, XWIN*2/3 + 260, PITCH, BLACK, -1);
    textout_centre_ex(buffer, font, f, XWIN*2/3 + 160, 2*PITCH, BLACK, -1);
    textout_centre_ex(buffer, font, g, XWIN*2/3 + 260, 2*PITCH, BLACK, -1);
    textout_centre_ex(buffer, font, h, XWIN*2/3 + 160, 3*PITCH, BLACK, -1);
    textout_centre_ex(buffer, font, i, XWIN*2/3 + 260, 3*PITCH, BLACK, -1);
}

void button_create(int x, int y, char s[10], int position, int color){
    // crete a button form x and y with the standard dimension defined previously

    int xl,xr,yl,yr,i;

    xl = x + WIDTH/2; 
    xr = x - WIDTH/2;
    yl = y;
    yr = y + TALL;
    
    if(position == SCREEN){
        rectfill(screen,xl,yl,xr,yr, color);
        textout_centre_ex(screen, font, s, x, y+10, 0, -1);

        for(i = 0; i < 4; i++){
            rect(screen, xl+i, yl-i,xr-i,yr+i, BLACK);
        }
    }
    else{
        rectfill(buffer, xl, yl, xr, yr, color);
        textout_centre_ex(buffer, font, s, x, y+10, 0, -1);

        for(i = 0; i < 4; i++){
            rect(buffer, xl+i, yl-i,xr-i,yr+i, BLACK);
        }
    }
}

int button_click(int x, int y, int xp, int yp){
    // return non false value if the button was clicked

    int click = 0;
    if(xp <= x+WIDTH/2 && xp >= x-WIDTH/2 && yp >= y && yp <= y+TALL) click = 1;
    return click;
}

void camera_init(){
    cam.x = 50;
    cam.y = YWIN/2;
    cam.h = 100;
    cam.w = 100;
}

void *motion(void *arg){
    // compute motion for the target

    struct point centroid, sign, contact;
    struct timespec t;
    int period, index,  local_ampl, local_mult;
    double local_vx, local_slope;

    index = get_index(arg);
    period = get_period(index);

    centroid.x = 50;
    centroid.y = YWIN/2;
    multiplier = 1;
    contact.x = 0;
    contact.y = YWIN/2;
    sign.x = 1;
    sign.y = 1;

    wait_for_activation(index);

    while(end == 0){

        sem_wait(&motion_sem);
        local_ampl = amplitude;
        local_mult = multiplier;
        local_slope = slope;
        local_vx = velocity_x;
        sem_post(&motion_sem);

        if(path == SIN){
            
            centroid.x = centroid.x + sign.x*local_vx*period;
            centroid.y = contact.y + (int)local_ampl*sin(0.3*(double)centroid.x);

            if(centroid.x >= XRBOUND || centroid.x <= XLBOUND){
                sign.x = -sign.x;
            }
        }

        if(path == LIN){
            contact.x = centroid.x;
            contact.y = centroid.y;

            centroid.x = centroid.x + sign.y*local_vx*period;
            centroid.y = (local_slope*sign.x*(centroid.x-contact.x)+contact.y);

            if(centroid.y >= YLBOUND){
                centroid.y = YLBOUND;
                sign.x = -sign.x;
                contact.x = centroid.x;
                contact.y = YLBOUND;
            }

            if(centroid.y <= YUBOUND){
                centroid.y = YUBOUND;

                sign.x = -sign.x;
                contact.x = centroid.x;
                contact.y = YUBOUND;
            }
            
            if(centroid.x >= XRBOUND){
                centroid.x = XRBOUND;

                contact.x = XRBOUND;
                contact.y = centroid.y;
                sign.x = -sign.x;
                sign.y = -sign.y;
            }

            if(centroid.x <= XLBOUND){
                centroid.x = XLBOUND;
                contact.x = XLBOUND;
                contact.y = centroid.y;
                sign.x = -sign.x;
                sign.y = -sign.y;
            }
        }

        if(path == MANUAL){
            if(key[KEY_W]) centroid.y -= local_mult*2;
            if(key[KEY_S]) centroid.y += local_mult*2;
            if(key[KEY_D]) centroid.x += local_mult*2;
            if(key[KEY_A]) centroid.x -= local_mult*2;

            if(centroid.x < XLBOUND) centroid.x = XLBOUND;
            if(centroid.x > XRBOUND) centroid.x = XRBOUND;
            if(centroid.y > YLBOUND) centroid.y = YLBOUND;
            if(centroid.y < YUBOUND) centroid.y = YUBOUND;
        }
        
        sem_wait(&obj_sem);
        
        obj.x = centroid.x;
        obj.y = centroid.y;
        
        sem_post(&obj_sem);

        if(deadline_miss(index)) dl.motion = get_dmiss(index);

        wait_for_period(index);
    }
    return NULL;
}

void *draw(void*arg){
    // task that draw on the screen
    struct timespec t;
    struct point bmenu, local_cam;
    int period, index, local_find;
    int x, y,i;

    index = get_index(arg);
    period = get_period(index);
    wait_for_activation(index);

    buffer = create_bitmap(XWIN, YWIN);
    bmenu.x = WIDTH/2 + PITCH;
    bmenu.y = PITCH;

    while(end == 0){
        
        sem_wait(&x_sem);
        local_cam.x = cam.x;
        sem_post(&x_sem);        
        
        sem_wait(&y_sem);
        local_cam.y = cam.y;
        sem_post(&y_sem);

        if(mouse_b & 1){
            x = mouse_x;
            y = mouse_y;
        }

        draw_hud();
        button_create(bmenu.x, bmenu.y, "BACK MENU", BUFFER, RED);

        if(button_click(bmenu.x, bmenu.y, x, y)) end = 1;

        if(background != DEFAULT){
            blit(bkground, buffer, 0, 0, 0, 2*PITCH + TALL + 2, bkground->w, bkground->h);
        }
        else rectfill(buffer, 0, YWIN, XWIN, 2*PITCH + TALL + 2, BLACK);

        sem_wait(&obj_sem);
        sem_wait(&color_sem);

        circlefill(buffer, obj.x, obj.y, 12, color);                                   // draw the target
        sem_post(&color_sem);
        sem_post(&obj_sem);

        sem_wait(&find_sem);
        local_find = find;
        sem_post(&find_sem);

        if(local_find == Y){
            for(i=0; i<4; i++){
                rect(buffer, local_cam.x-i, local_cam.y-i, local_cam.x+cam.w+i, local_cam.y+cam.h+i, GREEN);     //draw the camera window
            }
        } else {
            for(i=0; i<4; i++){
                rect(buffer, local_cam.x-i, local_cam.y-i, local_cam.x+cam.w+i, local_cam.y+cam.h+i, RED);     //draw the camera window
            }
        }

        blit(buffer, screen, 0, 0, 0, 0, buffer->w, buffer->h);
        rectfill(buffer, 0, YWIN, XWIN, 0, BKG);

        if(deadline_miss(index)) dl.draw = get_dmiss(index);

        wait_for_period(index);
    }
    return NULL;
}

void *scan_screen(void *arg){
    struct timespec t;
    struct point target, direction;

    int period, index, local_color;
    int x, y, c, old_ref_y[3], old_ref_x[3];
    long sum, xc, yc;

    direction.x = 0;
    direction.y = 0;

    index = get_index(arg);
    period = get_period(index);

    wait_for_activation(index);
    
    while(end == 0){
        sum = 0;
        xc = 0;
        yc = 0;
        target.x = 0;
        target.y = 0;

        sem_wait(&color_sem);
        local_color = color;
        sem_post(&color_sem);

        sem_wait(&x_sem);
        sem_wait(&y_sem);

        for(x=0; x<cam.w; x++){
            for(y=0; y<cam.h; y++){
                c = getpixel(screen, x+cam.x, y+cam.y);
                if(c == local_color){
                    sum++;
                    xc = xc + x;
                    yc = yc + y;
                }  
            }
        }

        sem_post(&x_sem);
        sem_post(&y_sem);

        if(sum > THRESHOLD){
            
            sem_wait(&x_sem);
            target.x = cam.x + xc/sum;
            ref.x = target.x - cam.w/2;
            old_ref_x[0] = ref.x;
            sem_post(&x_sem);

            sem_wait(&y_sem);
            target.y = cam.y + yc/sum;
            ref.y = target.y - cam.h/2;
            old_ref_y[0] = ref.y;                                           // store the reference
            sem_post(&y_sem);

            sem_wait(&find_sem);
            find = Y;
            sem_post(&find_sem);

            if((old_ref_x[1]-old_ref_x[0]) < 0) {       // compute the direction of the target
                direction.x = 1;
            }            
            else if((old_ref_x[1]-old_ref_x[0]) > 0){
                direction.x = -1;
            } 
            else {
                direction.x = 0;
            }

            if((old_ref_y[1]-old_ref_y[0]) < 0){
                direction.y = 1;
            }
            else if((old_ref_y[1]-old_ref_y[0]) > 0){
                direction.y = -1;
            }
            else {
                direction.y = 0;
            }

            old_ref_y[1] = old_ref_y[0];                                    // update the stored reference
            old_ref_x[1] = old_ref_x[0];
            
        }else {

            sem_wait(&find_sem);
            find = N;
            sem_post(&find_sem);

            if(direction.y == -1){  

                sem_wait(&y_sem);
                ref.y -= 5;
                if(ref.y < YUBOUND) direction.y = -direction.y;
                sem_post(&y_sem);
                
            } else if(direction.y == 1){

                sem_wait(&y_sem);
                ref.y += 5;
                if(ref.y > YLBOUND - cam.h) direction.y = -direction.y;
                sem_post(&y_sem);
            }

            if(direction.x == 1){

                sem_wait(&x_sem);
                ref.x += 5;
                if(ref.x > XRBOUND - cam.w) direction.x = -direction.x;
                sem_post(&x_sem);
                
            } else if(direction.x == -1){

                sem_wait(&x_sem);
                ref.x -= 5;
                if(ref.x < XLBOUND) direction.x = -direction.x;
                sem_post(&x_sem);
            }      
                            
        }

        if(deadline_miss(index)) dl.scan = get_dmiss(index);
        wait_for_period(index);
    }
}

void *user_interaction(void *arg){
    // task that collect the user input in the run section

    struct timespec t;
    int period, index, counter;
    int x, y, c;

    counter = 0;

    index = get_index(arg);
    period = get_period(index);
    wait_for_activation(index); 

    while(end == 0){

        if(key[KEY_L]){
            sem_wait(&motion_sem);
            velocity_x = velocity_x + 0.005;
            sem_post(&motion_sem);
        }
        if(key[KEY_K]){
            sem_wait(&motion_sem);
            velocity_x = velocity_x - 0.005;
            sem_post(&motion_sem);
        }

        if(key[KEY_1]){
            sem_wait(&motion_sem);
            path = SIN;
            sem_post(&motion_sem);
        }
        if(key[KEY_2]){
            sem_wait(&motion_sem);
            path = LIN;
            sem_post(&motion_sem);
        }
        if(key[KEY_3]){
            sem_wait(&motion_sem);
            path = MANUAL;
            sem_post(&motion_sem);
        }

        if(path == SIN){
            if(key[KEY_UP]){
                sem_wait(&motion_sem);
                amplitude++;
                sem_post(&motion_sem);
            }
            if(key[KEY_DOWN]){
                sem_wait(&motion_sem);
                amplitude--;
                sem_post(&motion_sem);
            }
        }
        else if(path == LIN){
            if(key[KEY_UP]){
                sem_wait(&motion_sem);
                slope += 0.1;
                sem_post(&motion_sem);
            }
            if(key[KEY_DOWN]) {
               sem_wait(&motion_sem);
               slope -= 0.1;
               sem_post(&motion_sem);
        }
        }
        else if(path == MANUAL){
            if(key[KEY_UP]) {
               sem_wait(&motion_sem);
               multiplier++;
               sem_post(&motion_sem);
        }
            if(key[KEY_DOWN]){
                sem_wait(&motion_sem);
                multiplier--;
                sem_post(&motion_sem);
            }
            if(multiplier <= 0) multiplier = 0;
        }

        if(key[KEY_P]){
            sem_wait(&c_sem);
            C += 0.5;
            sem_post(&c_sem);
        }
        if(key[KEY_O]){
            sem_wait(&c_sem);            
            C -= 0.5;
            sem_post(&c_sem);
        }
        
        counter--;                              //enter button debouncer
        if(counter < 0) counter = 0;

        if(key[KEY_ENTER]) {
            if(counter == 0){
                sem_wait(&color_sem);
                color++;
                if(color > 15) color = 1;
                sem_post(&color_sem);
                counter = 3;
            }
        }
        
        if(deadline_miss(index) != 0) dl.interaction = get_dmiss(index);

        wait_for_period(index);
    }   
}

void *motor_x(void *arg){
    struct timespec t;
    int period, index, i;
    double local_c;
    float e[3], x[5];
    float tau, k, periodms, p, a, b;

    for(i=0; i<3; i++){
        e[i] = 0;
    }

    for(i=0; i<5; i++){
        x[i] = 50;
    }

    tau = 0.01;
    k = 1;
    periodms = 0.05;
    p = 0.9;

    a = k*(periodms - tau + p*tau);
    b = k*(tau - p*periodms - p*tau);

    index = get_index(arg);
    period = get_period(index);

    wait_for_activation(index);

    while(end == 0){

        sem_wait(&c_sem);
        local_c = C;
        sem_post(&c_sem);

        sem_wait(&x_sem);
        e[2] = ref.x-cam.x;
        x[2] = local_c*(a*e[1] + b*e[0]) + (1+p)*x[1] - p*x[0];

        if(prediction == Y){
            x[3] = 2*x[2] + x[1];
            cam.x = (int)x[3];
        }else{
            cam.x = (int)x[2];
        }
        sem_post(&x_sem);

        x[0] = x[1];
        x[1] = x[2];

        if(prediction == Y) x[2] = x[3];

        e[0] = e[1];
        e[1] = e[2];

        if(deadline_miss(index)) dl.xmotor = get_dmiss(index);

        wait_for_period(index);
    }
    return NULL;
}

void *motor_y(void *arg){
    struct timespec t;
    int period, index, i;
    double local_c;
    float e[3], y[5];
    float tau, k, periodms, p, a, b;

    for(i=0; i<4; i++){
        e[i] = 0;
    }

    for(i=0; i<6; i++){
        y[i] = YWIN/2;
    }

    ref.y = YWIN/2;

    tau = 0.01;
    k = 1;
    periodms = 0.05;
    p = 0.9;

    a = k*(periodms - tau + p*tau);
    b = k*(tau - p*periodms - p*tau);

    index = get_index(arg);
    period = get_period(index);

    wait_for_activation(index);

    while(end == 0){

        sem_wait(&c_sem);
        local_c = C;
        sem_post(&c_sem);

        sem_wait(&y_sem);
        e[2] = ref.y-cam.y;
        y[2] = local_c*(a*e[1] + b*e[0]) + (1+p)*y[1] - p*y[0];
        if(prediction == Y){
            y[3] = 2*y[2] + y[1];
            cam.y = (int)y[3];
        }else{
            cam.y = (int)y[2];
        }

        sem_post(&y_sem);

        y[0] = y[1];                        //update reference
        y[1] = y[2];
        if(prediction == Y) y[2] = y[3];

        e[0] = e[1];
        e[1] = e[2];

        if(deadline_miss(index)) dl.ymotor = get_dmiss(index);

        wait_for_period(index);
    }
    return NULL;
}

void opt_box(int x, int y, char s[10]){
    //create the square for the option choice

    int xl,xr,yl,yr,i;
    xl = x + SIDE/2; 
    xr = x - SIDE/2;
    yl = y;
    yr = y + SIDE;

    for(i = 0; i < 4; i++){
        rect(screen, xl+i, yl-i,xr-i,yr+i,BLACK);
    }

    textout_centre_ex(screen, font, s, x, y-18, BLACK, -1);
}

int opt_click(int x, int y, int xp, int yp){
    // detect if the option box was clicked

    int xl,xr,yl,yr,i,click;
    click = 0;
    xl = x + SIDE/2; 
    xr = x - SIDE/2;
    yl = y;
    yr = y + SIDE;

    if(xp <= x+SIDE/2 && xp >= x-SIDE/2 && yp >= y && yp <= y+SIDE) click = 1;

    return click;
}

void draw_cross(int x,int y){
    // create a cross inside the option box to visually understand the choise

    int xl,xr,yl,yr;
    xl = x + SIDE/2; 
    xr = x - SIDE/2;
    yl = y;
    yr = y + SIDE;

    line(screen, xl, yl, xr, yr, BLACK);
    line(screen, xr, yl, xl, yr, BLACK);
}

void canc_cross(int x, int y){
    // delete a cross inside the option box

    int xl,xr,yl,yr;

    xl = x + SIDE/2; 
    xr = x - SIDE/2;
    yl = y;
    yr = y + SIDE;

    line(screen, xl, yl, xr, yr, BKG);
    line(screen, xr, yl, xl, yr, BKG);

    putpixel(screen,xr,yr,BLACK);
    putpixel(screen,xr,yl,BLACK);
    putpixel(screen,xl,yr,BLACK);
    putpixel(screen,xl,yl,BLACK);
}

void option_graphics(){
    //draw the line and writes on the screen in the option section
    int stripe[4], i;
    int base = 50;
    int col_text = XWIN/4;

    for(i=0; i<4; i++){
        stripe[i] = 90 + 100*i;
    }

    clear_to_color(screen,BKG);
    line(screen, 0, base, XWIN, base, BLACK);

    textout_centre_ex(screen, font,"EABLE PREDICTIONS: ", col_text, stripe[0], BLACK,-1);
    textout_centre_ex(screen, font,"BACKGROUND: ", col_text, stripe[1], BLACK, -1);
    textout_centre_ex(screen, font,"OBJECT MOTION:", col_text, stripe[2], BLACK, -1);

    for(i=0; i<2; i++){
        line(screen, 0, (stripe[i]+stripe[i+1])/2, XWIN,(stripe[i]+stripe[i+1])/2, BLACK);
    }

}

void option(){
    //open the options menu subsection

    int x,y,i;
    int result = 0;
    struct point bmenu;

    int row[4], col[5];

    bmenu.x = WIDTH/2 + PITCH;
    bmenu.y = PITCH;

    for(i=0; i<4; i++){
        row[i] = 80 + 100*i;
    }

    col[0] = XWIN/4 + 150;

    for(i=0; i<4; i++){
        col[i] = col[i-1] + BPITCH + 2*PITCH;
    }

    option_graphics();
    button_create(bmenu.x, bmenu.y, "BACK MENU", SCREEN, RED);
    
    opt_box(col[0], row[0], "yes");         // prediction options
    opt_box(col[1], row[0],"no");

    if(prediction = Y){
        draw_cross(col[0], row[0]);
    }else draw_cross(col[1], row[0]);

    opt_box(col[0], row[1], "city");        // background options
    opt_box(col[1], row[1], "forest");
    opt_box(col[2], row[1], "desert");
    opt_box(col[3], row[1], "black");

    switch (background){
        case CITY: 
            draw_cross(col[0], row[1]);
        break;
        case FOREST:
            draw_cross(col[1], row[1]);
        break;
        case DESERT:
            draw_cross(col[2], row[1]);
        break;
        default:
            draw_cross(col[3], row[1]);
        break;
    }

    opt_box(col[0], row[2], "sin");         //path motion option
    opt_box(col[1], row[2], "linear");
    opt_box(col[2], row[2], "manual");

    switch(path){
        case LIN:
            draw_cross(col[1], row[2]);
        break;
        case MANUAL:
            draw_cross(col[2], row[2]);
        break;
        default:
            draw_cross(col[0], row[2]);
        break;
    }

    while(result == 0){
        if(mouse_b & 1){
            x = mouse_x;
            y = mouse_y;

            if(button_click(bmenu.x, bmenu.y, x, y)) {
                result = 1;
            }

            if(opt_click(col[0], row[0], x, y)){
                draw_cross(col[0], row[0]);         //prediction
                canc_cross(col[1], row[0]);
                prediction = Y;
            }
            if(opt_click(col[1], row[0], x, y)){
                draw_cross(col[1], row[0]);         //prediction
                canc_cross(col[0], row[0]);
                prediction = N;
            }
            if(opt_click(col[0], row[1],x,y)){
                background = CITY;                  //background
                draw_cross(col[0],row[1]);
                canc_cross(col[2],row[1]);
                canc_cross(col[1],row[1]);
                canc_cross(col[3],row[1]);
            }
            if(opt_click(col[1], row[1],x ,y)){
                background = FOREST;                //background
                draw_cross(col[1], row[1]);
                canc_cross(col[0], row[1]);
                canc_cross(col[2], row[1]);
                canc_cross(col[3], row[1]);
            }
            if(opt_click(col[2], row[1], x, y)){
                background = DESERT;                //background
                draw_cross(col[2], row[1]);
                canc_cross(col[0], row[1]);
                canc_cross(col[1], row[1]);
                canc_cross(col[3], row[1]);
            }
            if(opt_click(col[3], row[1], x, y)){
                background = DEFAULT;               //background
                canc_cross(col[2], row[1]);
                canc_cross(col[0], row[1]);
                canc_cross(col[1], row[1]);
                draw_cross(col[3], row[1]);
            }
            if(opt_click(col[0], row[2], x, y)){
                path = SIN;                         //path
                draw_cross(col[0], row[2]);
                canc_cross(col[1], row[2]);
                canc_cross(col[2], row[2]);
            }
            if(opt_click(col[1], row[2], x,y)){
                path = LIN;                         //path
                draw_cross(col[1], row[2]);
                canc_cross(col[0], row[2]);
                canc_cross(col[2], row[2]);
            }
            if(opt_click(col[2], row[2], x, y)){
                path = MANUAL;                      //path
                draw_cross(col[2], row[2]);
                canc_cross(col[0], row[2]);
                canc_cross(col[1], row[2]);
            }
        }
    }  
}

void command(){
    // open the command subsection

    int x,y,i;
    int result = 0;
    struct point bmenu;

    int row[4], col[5];

    bmenu.x = WIDTH/2 + PITCH;
    bmenu.y = PITCH;

    for(i=0; i<4; i++){
        row[i] = 80 + 100*i;
    }

    col[0] = XWIN/2;

    clear_to_color(screen, BKG);
    textout_centre_ex(screen, font, "During the execution press 1 for sin-wave motion, 2 for linear, 3 for manual", XWIN/2, 200, BLACK, -1);
    textout_centre_ex(screen, font, "Press ENTER during the simulation to change the color of the target", XWIN/2, 250, BLACK, -1);
    textout_centre_ex(screen,font,"To increase the controller constant press P, to decrease it press O", XWIN/2, 300, BLACK,-1);
    textout_centre_ex(screen, font, "To increase the velocity of the target press L, to decrease it press K", XWIN/2, 350, BLACK, -1);
    textout_centre_ex(screen, font, "To increase the slope in sin motion press UP, to decrease it press DOWN", XWIN/2, 400, BLACK, -1);    
    
    button_create(bmenu.x, bmenu.y, "BACK MENU", SCREEN, RED);

    while(result == 0){
        if(mouse_b & 1){
            x = mouse_x;
            y = mouse_y;
            if(button_click(bmenu.x, bmenu.y, x, y)) result = 1;
        }
    }
}

void run(){
    // run the program

    int x, y,i;
    end = 0;

    dl.draw = 0;
    dl.motion = 0;
    dl.scan = 0;
    dl.interaction = 0;
    dl.xmotor = 0;
    dl.ymotor = 0;


    sem_init(&x_sem, 0, 1);
    sem_init(&y_sem, 0, 1);
    sem_init(&obj_sem, 0, 1);
    sem_init(&find_sem, 0, 1);
    sem_init(&color_sem, 0, 1);
    sem_init(&c_sem, 0, 1);
    sem_init(&motion_sem, 0, 1);

    switch (background){
        case CITY: 
            bkground = load_bitmap("city.bmp", NULL);
        break;
        case FOREST:
            bkground = load_bitmap("forest.bmp", NULL);
        break;
        case DESERT:
            bkground = load_bitmap("desert.bmp", NULL);
        break;
        default:
            background = DEFAULT;
        break;
    }


    if(background != DEFAULT && bkground == NULL){
        printf("File not found\n");
        exit(1);
    }

    camera_init();

    task_create(motion, 1, 60, 60, 20, ACT);
    task_create(draw, 2, 60, 60, 20, ACT);
    task_create(scan_screen, 3, 60, 60, 30, ACT);
    task_create(motor_x, 4, 80, 80, 30, ACT);
    task_create(motor_y, 5, 80, 80, 30, ACT);
    task_create(user_interaction, 6, 100, 100, 10, ACT);

    for(i=0; i<7; i++){
        wait_task_end(i);
    }

    sem_destroy(&x_sem);
    sem_destroy(&y_sem);
    sem_destroy(&obj_sem);
    sem_destroy(&c_sem);
    sem_destroy(&find_sem);
    sem_destroy(&color_sem);
    sem_destroy(&motion_sem);
    
    if(background != DEFAULT){
        destroy_bitmap(bkground);
    }
}

int menu(){
    // enter the menu where you can choose different buttons
    int result = 0;

    struct point click, option, command, run, end;
    
    run.x = XWIN/2;
    run.y = PITCH + TALL;
    
    option.x = XWIN/2;
    option.y = 2*PITCH + 3*TALL;

    command.x = XWIN/2;
    command.y = 3*PITCH + 5*TALL;

    end.x = XWIN/2;
    end.y = 4*PITCH + 7*TALL;

    clear_to_color(screen, BKG);
    button_create(option.x, option.y, "OPTIONS", SCREEN, CYAN);
    button_create(command.x, command.y, "COMMANDS", SCREEN, CYAN);
    button_create(run.x, run.y, "RUN SIMULATION", SCREEN, GREEN);
    button_create(end.x, end.y, "ABORT PROGRAM", SCREEN, RED);

    while(result == 0){
        if(mouse_b & 1){
            click.x = mouse_x;
            click.y = mouse_y;
            if (button_click(option.x, option.y, click.x, click.y))     result = OPTION;
            if (button_click(command.x, command.y, click.x, click.y))   result = COMMAND;
            if (button_click(run.x, run.y, click.x, click.y))           result = RUN;
            if (button_click(end.x, end.y, click.x, click.y))           result = ABORT;
        }
    }
    return result;
}

int main(){
    int section, bmenu;

    init();

    while(1){
        section = menu();
        
        if(section == OPTION) option();
        if(section == COMMAND) command();
        if(section == RUN) run(); 
        if(section == ABORT) break;
    }
    allegro_exit();

    return 0;
}