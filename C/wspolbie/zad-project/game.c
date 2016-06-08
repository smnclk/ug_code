#include "game.h"

#define LOOP_INTERVAL 15*1000

Display *mydisplay;
Window mywindow;
GC mygc;
XColor mycolorm, mycolorm1, dummy;

pthread_t tid1, tid2, tid3;
int p;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int klucz = 5535;
int* sharedPosition;
int pamiec;

void end(){
	printf("Usuwanie powiazan\n");
	shmdt(sharedPosition);
	shmctl(pamiec, IPC_RMID, 0);
	pthread_mutex_destroy(&lock);
}

void intHandler(int dummy) {
	end();
}

void *uploadPosition(void *argum){
	int xr,yr;
	int* isCancel = (int*)argum;
  
	while(*isCancel == 0){  
        xr = *(sharedPosition +0);
        yr = *(sharedPosition +1);
        *(sharedPosition +4) = xr;
        *(sharedPosition +5) = yr;
		usleep(LOOP_INTERVAL);
	}
	printf("End upload\n");
}

void *downloadPosition(void *argum){
	int xr,yr;
	int* isCancel = (int*)argum;
  
	while(*isCancel == 0){  
        xr = *(sharedPosition +4);
        yr = *(sharedPosition +5);
        *(sharedPosition +2) = xr;
        *(sharedPosition +3) = yr;
		usleep(LOOP_INTERVAL);
	}
	printf("End download\n");
}

void *draw(void *argum){
	int xr,yr;
	int* isCancel = (int*)argum;
  
	while(*isCancel == 0){  
        pthread_mutex_lock(&lock);
        xr = *(sharedPosition +2);
        yr = *(sharedPosition +3);
        XClearArea(mydisplay, mywindow, 0, 0, 500, 500, 0);
		XSetForeground(mydisplay, mygc, mycolorm.pixel);
		XFillArc(mydisplay, mywindow, mygc, xr-(40/2), yr-(40/2), 40, 40, 0, 360*64);
		XFlush(mydisplay);
		pthread_mutex_unlock(&lock);
		usleep(LOOP_INTERVAL);
	}
	printf("End draw\n");
}

void main(){
	signal(SIGINT, intHandler);
	p = getpid();

	XInitThreads();
	mydisplay = XOpenDisplay("");

	int myscreen;
	myscreen = DefaultScreen(mydisplay);

	Visual *myvisual;
	myvisual = DefaultVisual(mydisplay, myscreen);

	int mydepth;
	mydepth = DefaultDepth(mydisplay, myscreen);
	XSetWindowAttributes mywindowattributes;
	mywindowattributes.background_pixel = XWhitePixel(mydisplay, myscreen);
	mywindowattributes.override_redirect = False;
	mywindowattributes.backing_store = Always;
	mywindowattributes.bit_gravity = NorthWestGravity;

	mywindow = XCreateWindow(mydisplay, XRootWindow(mydisplay, myscreen),
		0, 0, 500, 500, 10, mydepth, InputOutput,
		myvisual, CWBackingStore | CWBackingPlanes| CWBitGravity |
		CWBackPixel | CWOverrideRedirect, &mywindowattributes);

	XSelectInput(mydisplay, mywindow, KeyPressMask | ButtonPressMask | ButtonMotionMask);

	Colormap mycolormap;
	mycolormap = DefaultColormap(mydisplay, myscreen);                 

	XAllocNamedColor(mydisplay, mycolormap, "red", &mycolorm, &dummy);

	XAllocNamedColor(mydisplay, mycolormap, "blue", &mycolorm1, &dummy);

	XMapWindow(mydisplay, mywindow);

	mygc = DefaultGC(mydisplay, myscreen);

	pamiec = shmget(klucz, sizeof(int) *6, 0777 | IPC_CREAT);
	sharedPosition = shmat(pamiec, 0, 0);

	int cancelDraw = 0;
	pthread_create(&tid1, NULL, draw, &cancelDraw);

	int cancelUp = 0;
	pthread_create(&tid2, NULL, uploadPosition, &cancelUp);

	int cancelDown = 0;
	pthread_create(&tid3, NULL, downloadPosition, &cancelDown);

	XEvent myevent;
	int xw, yw;
	while (1){
		XNextEvent(mydisplay, &myevent);
		switch (myevent.type){
			case ButtonPress:
				xw = myevent.xbutton.x;
				yw = myevent.xbutton.y;
				*(sharedPosition +0) = xw;
				*(sharedPosition +1) = yw;
				break;
			case MotionNotify:
				xw = myevent.xmotion.x;
				yw = myevent.xmotion.y;
				*(sharedPosition +0) = xw;
				*(sharedPosition +1) = yw;
				break;
			case KeyPress:
				// Only Esc close window
				if(myevent.xkey.keycode == 9){
					cancelDraw = 1;
					cancelUp = 1;
					cancelDown = 1;
					XCloseDisplay(mydisplay);
					end();
					exit(0);
				}
				break;
		}
	}
}
