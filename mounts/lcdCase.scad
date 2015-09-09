th=2.5;
c=.5;

lcdW=97;
lcdH=40;
// add extra height to accommodate the cover
lcdD=23+2*th;

cbH=60;
cbW=108.2;

cpW=10;

standoff=7.25;

w = cbW/2+cpW/2+th/2;
l_mount = 15;
h_mount = 4;
w_mount= 20;
// For making rounded corners
module fillet(r, h) {
    translate([r / 2, r / 2, 0])
        difference() {
            cube([r + 0.01, r + 0.01, h], center = true);
            translate([r/2, r/2, 0])
                cylinder(r = r, h = h + 1,$fn = 50, center = true);
        }
}


module front(){
	difference(){
		cube([cbW+cpW+th*2, cbH+th*2, lcdD+th*2], center=true);
		//body cutout
		translate([0,0,-th])cube([cbW+cpW+c*2, cbH+c*2, lcdD+th*2], center=true);
		//slot
		translate([0,-2,-lcdD])cube([cbW+cpW+c*2, cbH+c*2+0.1, lcdD+th*2], center=true);
		//lcd cutout
		translate([-cpW/2,0,0])
		cube([lcdW+c*2, lcdH+c*2, lcdD*2], center=true);
        //wire cutout
        translate([cbW/2+cpW/2+th/2, 20, -lcdD/2+2*th+3])
        rotate([0,90,0]) cylinder(r=3.0, h=th*3, center=true, $fn=50);
        translate([cbW/2+cpW/2+th/2, 20, -lcdD/2])
        rotate([0,180,0]) cube([th*3, 6, lcdD-10], center=true);

	}

    //wall mount
    /*
    union(){
        translate([w,w_mount/2,lcdD/2+th]) rotate([180,0,0])  cube([l_mount,w_mount,h_mount]);
        // add rounded corner
        translate([w+th/2,0,lcdD/2-h_mount/2+th/2]) rotate([90,90,0])
        fillet(2*th,w_mount+0.1);
    }
    */
    //standoff
	translate([-cpW/2,0,lcdD/2-standoff/2]){
		for (s=[-1,1])
			for (t=[-1,1])translate([s*(7.9+lcdW*0.4),t*(4+lcdH*0.6),0])difference(){
				cylinder(r=3.0, h=standoff, center=true, $fn=50);
				cylinder(r=1.5, h=standoff+1, center=true, $fn=50);
			}
	}
	//flange
	translate([0,cbH/2+1,-lcdD/2-th/2])rotate([180,0,0])rotate([0,90,0])cylinder(r=2,h=cbW+cpW+th+c*2,$fn=3,center=true);
	for(s=[-1,1]){
		translate([s*(cbW+cpW+2)/2,0,-lcdD/2-th/2+0.01])rotate([0,-90,0])rotate([90,0,0])cylinder(r=2,h=cbH+2*th,$fn=3,center=true);
	}
}

module cover(){
	difference(){
		union(){
			//flange
			cube([cbW+cpW+c*2, cbH+c*2+0.1, th], center=true);

			//base
			translate([0,0,th])cube([cbW+cpW+c*2-th*2, cbH+c*2+0.1-th*2, th], center=true);
		}
		for (s=[-1,0,1])for(t=[-1,1])
			translate([s*cbW*.35,t*cbH/4.5,0])cube([22,15,10],center=true);
		//translate([0,0,-th*0.0])cube([cbW+cpW+c*2-th*2-6, cbH+c*2-th*2-6, th*1.01], center=true);
	}
}

mirror([0,0,1])front();
translate([0,cbH+10,-(lcdD+th)/2])cover();