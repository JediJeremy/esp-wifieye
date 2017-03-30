/*
    WiFi Eye - by Jeremy Lee, Dec 2016
               with assistance from Naiomi Wu
               Heavily inspired by the Doctor Strange "Eye of Agomotto" 
               prop designed by Alexandra Byrne and Barry Gibbs
               
    
    This design is public domain, share and enjoy.
    
    Note this is a very complicated model, and it will take a while
    (perhaps several minutes) to load and render the preview. 
    
    Go get a cup of coffee, be patient. There's a lot of curves to be intersected
    and render caches to be prepared. It gets faster once it's all ready.
    
*/

// choose from 1..7 for print platters, 0 for display mode
platter = 0;

// switch to high resolution if we are preparing the prints
curve_resolution = (platter==0) ? 30 : 90;

// hint for small segments (doesn't work that well)
$fs=0.5;

animation = "1";

if(animation=="1") {
	core_op = 1;
	
	neopixel_op = mix($t, 0.0,0.1, 0,1);	
	
	lids_op = mix($t, 0.1,0.2, 0,1);
	servo_op = mix($t, 0.2,0.3, 0,1);	
	servo_x = mix($t, 0.3,0.4, 10,0);	
	lids_z = mix($t, 0.4,0.5, 20,0);	
	
	deck_op = mix($t, 0.6,0.7, 0,1);	
	deck_y = mix($t, 0.6,0.7, 10,0);	
	boards_op = mix($t, 0.65,0.75, 0,1);	

	batt_op = mix($t, 0.65,0.75, 0,1);	
	batt_y = mix($t, 0.65,0.75, -20,0);	
	
	
	face_op = mix($t, 0.8,0.9, 0,1);	
	face_z = mix($t, 0.8,0.9, 30,0);	
	
	
	lid_angle=mix($t, 0.75,0.95, 50,1);
	servo_angle=10;
    // translate([0,-22,-1]) component_battery();
	
    if(core_op) color("gold",core_op) render() eye_core();
    if(neopixel_op) translate([0,0,03]) component_neopixel(opacity=neopixel_op);
	if(servo_op) {
		translate([-24-servo_x,0,lids_z]) rotate([0,0,0])  component_servo(angle=lid_angle+servo_angle, opacity=servo_op);
		translate([24+servo_x,0,lids_z]) rotate([0,0,180]) component_servo(angle=lid_angle+servo_angle, opacity=servo_op);
	}
    if(lids_op) translate([0,0,lids_z]) color("orange",lids_op) eye_lids(lid_angle,servo_angle=servo_angle,offset_x=servo_x/2);
		
	if(batt_op) translate([0,-20+batt_y,-6]) rotate([0,90,0]) color("blue",batt_op) cylinder(d=19,h=65,center=true); // battery round
    if(deck_op) translate([0,deck_y,0]) color("gold",deck_op) render() middle_deck();
	if(boards_op) translate([0,deck_y,0]) {
		translate([16,22,2]) rotate([0,0,-190]) component_esp8266(opacity=boards_op);
		color([0.2,0.4,0.2],boards_op)  translate([-16,22,0]) rotate([0,0,110]) cube([12,30,1],center=true); // battery charger
	}
	translate([0,0,face_z]) {
		if(face_op) color("orange",face_op) amulet_body();
		if(face_op) color("orange",face_op) amulet_crossbeams();    
	}
	
} else if(animation=="2") {

} else if(platter == 0) {
    // display mode
    rotate([90,0,0]) {
        display_components(05);
        color("yellow",0.35) display_assembly();
    }
} else if(platter == 1) {
    // print amulet body
    amulet_body();
} else if(platter == 2) {
    // print amulet crossbeams
    amulet_crossbeams();    
} else if(platter == 3) {
    rotate([45,0,0]) eye_lid();
} else if(platter == 4) {
    // print eye core
    color("brown") eye_core();
} else if(platter == 5) {
    // electronics cover plate
    color("green") middle_deck();
} else if(platter == 6) {
    // print platter 6
    rotate([0,0,0]) holder_stand(build_support=true);
} else if(platter == 7) {
    // print platter 7
    rotate([-90,0,0]) necklace_cap();
}    


module letter(l,h=2,size=10) {
  // Use linear_extrude() to make the letters 3D objects as they
  // are only 2D shapes when only using text()
  linear_extrude(height = h) {
    text(l, size = size, font = "SimSun", halign = "center", valign = "center", $fn = 16);
  }
}

module lid_ring(id=2,od=4) {
    render(convexity = 2) rotate([0,90,0]) difference() {
      cylinder(h=h,d=od, $fn=curve_resolution, center=true);
      cylinder(h=h+2,d=id, $fn=curve_resolution, center=true);
    }
}


module lid_cone(h=2,od1=4,od2=3) {
    rotate([0,-90,0]) linear_extrude(height=h, scale=od2 / od1) circle(d=od1); // conical support
}

module eye_lid_cut_base() {
  translate([0,0,-33.0])  rotate([-8,0,0]) cube([60,60,60], center=true);
  translate([0,-11.5,-40])  rotate([-45,0,0]) cube([80,80,60], center=true);
}

module eye_lid_cut_faces() {
  translate([0,30,0])    cube([60,60,60], center=true);
  eye_lid_cut_base();
}

module eye_lid_servo_space(servo_angle=30) {
    rotate([servo_angle,0,0]) {
        // servo horn space
        translate([-13,-5.2,0])    cube([4.25,12,2.4], center=true);
        // screw holes
        // translate([-11,-5.0,0]) rotate([0,90,0]) cylinder(h=4.0,d=0.6, center=true);
        translate([-11,-6.0,0]) rotate([0,90,0]) cylinder(h=4.0,d=0.8, center=true);
    }
}

module eye_lid(servo_angle=10) {
    // lid shell
    difference() {
      union() {
        // outer shell
        sphere(d=24, $fn=curve_resolution);
        // servo horn base
        rotate([servo_angle,0,0]) translate([-10,-4,0])    cube([5,10,5], center=true);
      }
      sphere(d=20, $fn=curve_resolution);
      eye_lid_cut_faces();
      translate([-12,0,0])   rotate([0,90,0]) cylinder(h=6,d=3.5, center=true);
      translate([-13,0,0])   rotate([0,90,0]) cylinder(h=5,d=4.6, center=true);
      translate([+13,0,0])   lid_cone(h=4.0,od1=10,od2=6); // rotate([0,90,0]) cylinder(h=6,d=6.2, center=true);
      // opposing slip ring
      translate([-9.4,0,0]) lid_ring(id=3.0,od=6.2,h=1.2);
      // servo horn space
      eye_lid_servo_space(servo_angle=servo_angle);
      // extra space for opposing servo horn
      translate([11.7,-3.0,-2]) rotate([0,25,60])    cube([4,6,6], center=true); 
    }
    // servo spline connection
    translate([-10.4,0,0]) lid_ring(id=3.5,od=6,h=1);
    difference() {
        translate([-11.4,0,0]) lid_ring(id=4.6,od=6,h=3);
        eye_lid_servo_space(servo_angle=servo_angle);
        // eye_lid_cut_base();
    }
    // registration bump
    translate([11.0,0,0]) lid_cone(h=2.0,od1=3,od2=4);
    // registration bump support
    difference() {
        translate([+9.2,0,0]) rotate([0,90,0]) cylinder(h=1.0,d=10.0, center=true);
        eye_lid_cut_faces();
    }
    difference() {
        translate([+9.2,0,0]) rotate([0,90,0]) cylinder(h=1.0,d=5.0, center=true);
        eye_lid_cut_base();
    }
}

module eye_lid_1() {
   render() eye_lid();
}

module eye_lid_2() {
   rotate([0,0,180]) eye_lid_1();
}

module eye_lids(a=10,servo_angle=10,offset_x=0) {
	translate([-offset_x,0,4]) rotate([a,0,0]) eye_lid_1(servo_angle=servo_angle);
	translate([+offset_x,0,4]) rotate([-a,0,0]) eye_lid_2(servo_angle=servo_angle);
}

module amulet_face() {
    scale([1,0.70,0.2]) difference() {
        sphere(d=150, $fn=curve_resolution);
        translate([0,0,-55])    cube([200,200,100],center=true);
    }
}

module amulet_face_inset() {
    translate([0,0,-1]) amulet_face();
    translate([0,0,-48])    cube([220,220,100],center=true);
}


module eye_fold(a=1.0) {
     scale([a,a,1.0]) difference() {
        minkowski() {
            scale([0.7,0.5,0.2]) difference() {
                sphere(d=80);
                translate([0,0,-60])    cube([200,200,100],center=true);
            }
            scale([2,1,1]) rotate([0,0,45])    cube([10,10,2], center=true);
        }
        translate([0,0,10]) minkowski() {
            scale([0.7,0.5,0.2]) difference() {
                sphere(d=80);
                translate([0,0,50])    cube([200,200,100],center=true);
            }
            scale([2,1,1]) rotate([0,0,45])    cube([10,10,2], center=true);
        }
    }
}

module eye_corners(a=1.0) {
     scale([a,a,1.0])  difference() {
         minkowski() {
            scale([1,0.5,0.2]) difference() {
                sphere(d=80);
                translate([0,0,-50])    cube([200,200,100],center=true);
            }
            scale([2,1,1]) rotate([0,0,45])    cube([10,10,2], center=true);
        }
        translate([0,0,14]) minkowski() {
            scale([1,0.5,0.2])  difference() {
                sphere(d=80);
                translate([0,0,50])    cube([200,200,100],center=true);
            }
            scale([2,1,1]) rotate([0,0,45])    cube([10,10,2], center=true);
        }
    }
    translate([0,0,3])  minkowski() {
        difference() {
            cylinder(d=30, h=7, center=true, $fn=curve_resolution);
            cylinder(d=26, h=10, center=true, $fn=curve_resolution);
        }
        sphere(d=2);
    }
}


module eye_folds() {
    render(convexity = 2) translate([0,0,1])  union() {
        translate([0,0,-1]) eye_fold(a=0.7);
        eye_corners(a=0.6);
    }    
}

module etch_1() {
    translate([0,-20,0]) scale([1.05,0.9,1]) difference() {
        cylinder(d=151, h=100, center=true, $fn=curve_resolution);
        cylinder(d=148, h=100, center=true, $fn=curve_resolution);
        translate([0,0,60])      cube([200,200,100],center=true);
        translate([0,0,-50])     cube([200,200,100],center=true);
        translate([0,-100,0])    cube([200,200,100],center=true);
        cube([7,200,100],center=true);
    }
    translate([0,20,0]) scale([1.05,0.9,1]) difference() {
        cylinder(d=151, h=100, center=true, $fn=curve_resolution);
        cylinder(d=148, h=100, center=true, $fn=curve_resolution);
        translate([0,0,60])     cube([200,200,100],center=true);
        translate([0,0,-50])    cube([200,200,100],center=true);
        translate([0,100,0])    cube([200,200,100],center=true);
        cube([7,200,100],center=true);
    }
}
module etch_2() {
    difference() {
        cylinder(d=90, h=100, center=true, $fn=curve_resolution);
        cylinder(d=88, h=100, center=true, $fn=curve_resolution);
        translate([0,0,70])     cube([200,200,100],center=true);
        translate([0,0,-50])    cube([200,200,100],center=true);
        
    }
}

module etch_3() {
    difference() {
        scale([1.1,0.7,1]) difference() {
            cylinder(d=90, h=100, center=true, $fn=curve_resolution);
            cylinder(d=88, h=100, center=true, $fn=curve_resolution);
            translate([0,0,70])    cube([200,200,100],center=true);
            translate([0,0,-50])    cube([200,200,100],center=true);
        }
        cylinder(d=90, h=100, center=true, $fn=curve_resolution);
    }
}

module etch_4() {
    difference() {
        union() {
            // central vertical double-bar
            translate([-4,0,0])    cube([1,200,50],center=true);
            translate([+4,0,0])    cube([1,200,50],center=true);
            // rotated bars
            rotate([0,0,-65])    cube([1,200,50],center=true);
            rotate([0,0,-40])    cube([1,200,50],center=true);
            rotate([0,0,-20])    cube([1,200,50],center=true);
            rotate([0,0,20])    cube([1,200,50],center=true);
            rotate([0,0,40])    cube([1,200,50],center=true);
            rotate([0,0,65])    cube([1,200,50],center=true);
        }
        cylinder(d=90, h=120, center=true);
    }
    // horizontal (broken) bar
    difference() {
        cube([200,1,50],center=true);
        scale([1.1,0.7,1]) cylinder(d=90, h=120, center=true);
    }
}

module character_etchings() {
    // the ancient glyphs of Wu:
    // 机械妖姬 是最棒的
    // right hand side
    rotate([0,0,36]) translate([50,0,0]) letter("机",20,7);
    rotate([0,0,13]) translate([56,0,0]) letter("械",20,10);
    rotate([0,0,-13])  translate([56,0,0]) letter("妖",20,10);
    rotate([0,0,-36])  translate([50,0,0]) letter("姬",20,7);
    // left hand side
    rotate([0,0,-36]) translate([-50,0,0]) letter("是",20,7);
    rotate([0,0,-13]) translate([-56,0,0]) letter("最",20,10);
    rotate([0,0,13])  translate([-56,0,0]) letter("棒",20,10);
    rotate([0,0,36])  translate([-50,0,0]) letter("的",20,7);
}


module etchings() {
    difference() {
        union() {
            etch_2();
            etch_3();
            etch_4();
            etch_1();
            color("brown") character_etchings();
        }
        amulet_face_inset();
    }
}


module character_infill(h=2,z=4) {
    // the ancient glyphs of Wu:
    // 机械妖姬 是最棒的
    // top
    rotate([0,0,40]) translate([0,25,z]) letter("机",h,12);
    rotate([0,0,17]) translate([0,22,z]) letter("械",h,12);
    rotate([0,0,-17])  translate([0,22,z]) letter("妖",h,12);
    rotate([0,0,-40])  translate([0,25,z]) letter("姬",h,12);
    // bottom
    rotate([0,0,-36]) translate([0,-25,z]) letter("是",h,12);
    rotate([0,0,-13]) translate([0,-22,z]) letter("最",h,12);
    rotate([0,0,13])  translate([0,-22,z]) letter("棒",h,12);
    rotate([0,0,36])  translate([0,-25,z]) letter("的",h,12);
    // infill ring
    translate([0,0,z+1]) difference() {
        cylinder(d=47, h=2, center=true);
        cylinder(d=44, h=4, center=true);
    } 
}

module amulet_crossbeams_slicecurve(r1=100, r2=98) {
    intersection() {
        translate([0,0,2]) cylinder(h=20,d=58); // dial space
        translate([0,0,-86]) scale([1,0.8,1]) difference() {
            sphere(r=r1, $fn=curve_resolution);
            sphere(r=r2, $fn=curve_resolution);
        }
    }
}

module amulet_crossbeams_slice(r1=100, r2=98) {
    render(convexity = 2) intersection() {
        // cross surface
        amulet_crossbeams_slicecurve(r1=r1, r2=r2);
        // cross beams
        union() {
            translate([-14,0,10]) rotate([40,0,85]) cube([100,2,50],center=true);
            translate([16,-4,10]) rotate([-40,0,50]) cube([100,2,50],center=true); 
            translate([-15,-15,10]) rotate([-25,0,-20]) cube([100,2,50],center=true); 
            translate([-8,-8,10]) rotate([46,0,-20]) cube([100,2,50],center=true); 
        }
    }
}

module amulet_crossbeams() {
    // the crossbeams
    amulet_crossbeams_slice(r1=100, r2=98.6);
}

// Here we go, the primary assembly of the amulet face
module amulet_etched() {
    // the etched concave surface, originally modelled a little too big.
    scale(0.7) render(convexity = 2) {
        difference() {
            amulet_face();
            union() {
                etchings();
                translate([0,0,6]) cylinder(h=40,d=80, $fn=curve_resolution);
                translate([0,0,-5]) cylinder(h=40,d=34, $fn=curve_resolution);
            }
        }
    }
    // infill around the dial
    character_infill(z=3);
    // eye corner 'folds' to cover the servos
    translate([0,0,5]) scale([1,1,0.5]) eye_folds();
}

// arrow-like block to wrap the zip-tie around
module eye_core_tie_end() {
    translate([3,0,-1.5]) cube([3,8,6],center=true); // tie block        
    difference() {
        translate([-1,0,-3]) cube([10,8,3],center=true); // tie block        
        translate([0,-5,-8]) rotate([45,0,0]) cube([3,10,10],center=true); // zip-tie cut 1
        translate([0,5,-8]) rotate([45,0,0]) cube([3,10,10],center=true); // zip-tie cut 2
        translate([-10,5,-4]) rotate([0,-45,0]) cube([10,20,20],center=true); // zip-tie cut 2
    }
}

// retaining arm to hold the plate under the core
module eye_core_plate_arm() {
    difference() {
        translate([0,0,-3]) cube([6,12,3],center=true); // tie block        
        translate([0,4,-12]) rotate([0,30,90]) cube([10,40,40],center=true); // 30 degree chamfer
        translate([0,1.5,-12]) rotate([0,45,90]) cube([10,40,40],center=true); // 45 degree chamfer
    }
}

// the carrier for the mechatronics components (servos and lights)
module eye_core() {
    translate([0,18,-2.5]) eye_core_plate_arm();
    translate([30,12,-2.5]) eye_core_plate_arm();
    translate([-30,12,-2.5]) eye_core_plate_arm();

    difference() {
        union() {
            translate([0,0,2.0]) linear_extrude(height=1, scale=0.9) circle(d=16); // conical neopixel platform
            // translate([0,0,2.5]) cylinder(h=1,d=16,center=true); // neopixel platform
            translate([0,0,2]) rotate([0,180,0]) linear_extrude(height=9, scale=0.1) circle(d=16); // conical support
            translate([0,0,-2]) cube([12,5,9],center=true); // platform support
            translate([0,0,-6.5]) cube([75,13.0,1],center=true); // support beam
        }
        translate([0,6,2]) rotate([-30,0,0]) cube([4,3.5,30],center=true); // cable slot space
        translate([10,0,2]) cube([4.5,6,10],center=true); // slip ring space 1
        translate([-10,0,2]) cube([4.5,6,10],center=true); // slip ring space 2
        translate([+25,0,0]) cube([21,6.4,15],center=true); // servo bodies
        translate([-25,0,0]) cube([21,6.4,15],center=true); // servo bodies
    }
    difference() {
        translate([0,0,-7]) rotate([0,0,0]) linear_extrude(height=1, scale=1) circle(d=26); // eye shroud base
        component_battery_space();
    }
    difference() {
        union() {
            translate([-39,0,-2.5]) rotate([0,0,0]) eye_core_tie_end();
            translate([39,0,-2.5]) rotate([0,0,180]) eye_core_tie_end();            
            translate([0,-5,-3.5]) cube([75,3.2,6],center=true); // left servo clip 2
            translate([0,5,-3.5]) cube([75,3.2,6],center=true);  // left servo clip 3
            
            // eye shroud enclosure
            difference() {
                union() {
                    translate([-13,0,-3.75]) cube([10,20,6.5],center=true); // left servo clip 1            
                    translate([13,0,-3.75]) cube([10,20,6.5],center=true); // right servo clip 1
                    intersection() {
                        cube([26,100,100],center=true); // limit to center
                        translate([0,0,-7]) rotate([0,0,0]) linear_extrude(height=11.5, scale=1.32) circle(d=26); // conical support
                    }
                }
                translate([0,13,1]) rotate([-50,0,0]) cube([4,6,30],center=true); // cable slot space
                translate([0,0,0]) rotate([0,0,0]) cube([50,6.5,30],center=true); // servo space
                component_battery_space();
            }
            
        }
        translate([-10,0,4])   rotate([0,90,0]) cylinder(h=10,d=20, center=true); // lid ring 1
        translate([+10,0,4])   rotate([0,90,0]) cylinder(h=10,d=20, center=true); // lid ring 2
        translate([0,0,5]) scale([1.3,1,1]) sphere(d=26, $fn=curve_resolution); // eye sphere space
        core_zipties();
    }
    
}

// cover for the electronics
module middle_deck() {
    difference() {
        translate([0,0,-4]) scale([1.0,0.7,1.0]) linear_extrude(height=3.5, scale=1.04) circle(d=98, $fn=curve_resolution); // cone
        translate([0,-54,0]) cube([200,100,100],center=true); // lower cutoff
        difference() {
            translate([0,-2,-3]) scale([1.0,0.7,1.0]) linear_extrude(height=4, scale=1.04) circle(d=96, $fn=curve_resolution); // cone
            translate([0,-41,0]) cube([200,100,100],center=true); // lower cutoff
        }
        // core space 
        translate([0,0,0]) cube([75.6,14,100],center=true); 
        translate([0,0,0]) cube([37,21,100],center=true); 
        cylinder(h=10,d=30,center=true); 
        // and the zip-ties
        core_zipties();
        // wiring spaces
        translate([-43,0,0]) cube([2,20,4],center=true); 
        translate([+43,0,0]) cube([2,20,4],center=true); 
    }   
}


module component_battery() {
    color("blue") cube([32,17,6],center=true);
}


module component_esp8266(opacity=1) {
    color([0.2,0.4,0.2],opacity) translate([0,0,-1])    cube([25,17,0.5],center=true);
    color("silver",opacity) translate([4,0,0])    cube([15,11,2],center=true);
}


module component_neopixel(opacity=1) {
    color([0.2,0.4,0.2],opacity) translate([0,0,0])    cube([15,10,0.5],center=true);
    color("white",opacity) translate([0,0,0.5])    cube([5,5,1],center=true);
}


module component_servo(angle=0,opacity=1) {
    // servo body
    color("silver",opacity) {
        translate([0,0,0]) cube([15,6,13],center=true);
        translate([8,0,4]) rotate([0,90,0]) cylinder(h=2,d=3.5,center=true);
        difference() {
            translate([8,0,4]) rotate([0,90,0]) cylinder(h=5,d=3,center=true);
            translate([10,0,4]) rotate([0,90,0]) cylinder(h=6,d=1,center=true);
        }
        translate([-8,0,-4]) rotate([0,90,0]) cylinder(h=5,d=5,center=true);
        translate([-8,0,-4]) cube([5,2,6],center=true);
    }
    // servo horn
    color("white",opacity) translate([10,0,4]) rotate([angle,0,0]) {
        difference() {
            union() {
                difference() {
                    translate([1,0,0]) rotate([0,90,0]) cylinder(h=2,d=4,center=true);
                    translate([0,0,0]) rotate([0,90,0]) cylinder(h=2,d=2.5,center=true);
                }
                translate([1.5,-2.5,0]) cube([1,8,2],center=true);
                translate([1.5,-6.5,0]) rotate([0,90,0]) cylinder(h=1,d=2,center=true);
            }
            // holes
            translate([1,0,0]) rotate([0,90,0]) cylinder(h=4,d=0.8,center=true);
            translate([1,-5,0]) rotate([0,90,0]) cylinder(h=4,d=0.5,center=true);
            translate([1,-6,0]) rotate([0,90,0]) cylinder(h=4,d=0.5,center=true);
        }
    }
}

module component_battery_space() {
    // translate([0,-22,-0.2]) cube([35,18,6],center=true); // battery flat
    translate([0,-20,-6]) rotate([0,90,0]) cylinder(d=19,h=65,center=true); // battery round
}

module component_pockets() {
    translate([0,0,5]) sphere(d=24); // eye hole
    translate([0,0,-5]) cylinder(d=35,h=10); // eye hole

    // translate([-40,0,-0.2]) rotate([0,0,0]) cube([18,25,2],center=true); // battery charger
    component_battery_space();
    
    translate([19,18.5,-0.2]) rotate([0,0,-20]) cube([30,18,6],center=true); // esp8266
    translate([-19,19,-0.2]) rotate([0,0,22]) cube([40,12,6],center=true); // pdb
    translate([5,25,-0.2]) rotate([0,0,0]) cube([20,6,6],center=true); // esp8266 - pdb cable space
    
    
    translate([0,0,0]) cube([65,6.4,13],center=true); // servo bodies

    translate([25.5,10.7,-4]) rotate([0,0,-60]) cube([20,4,13],center=true); // servo 1 cable space
    translate([-25.5,10.7,-4]) rotate([0,0,60]) cube([20,4,13],center=true); // servo 2 cable space
        
    // servo horn space 1
    translate([-12,0,4])   rotate([0,90,0]) cylinder(h=4,d=7, center=true); 
    translate([-11,-5,3.5])   rotate([5,0,0]) cube([6,10,6],center=true); // 
    // servo horn space 2
    translate([+12,0,4])   rotate([0,90,0]) cylinder(h=4,d=7, center=true); 
    translate([+11,-5,3.5])   rotate([5,0,0]) cube([6,10,6],center=true); // 

}

module attachment_ring() {
    render(convexity = 2)  minkowski() {
        // ring created from intersection of cylinders
        difference() {
            cylinder(d=8, h=1, center=true, $fn=curve_resolution);
            cylinder(d=6, h=10, center=true, $fn=curve_resolution);
        }
        // expand with a sphere
        sphere(d=2);
    }
}

module ziptie_arch(d=10,h=20,width=3,thick=1.5) {
    render(convexity = 2) {
        // difference of two sideways cylinders and a clube for the arch
        difference() {
            rotate([0,90,0]) cylinder(h=width,  d=d,   center=true); // outer curve
            rotate([0,90,0]) cylinder(h=width+2,d=d-(thick*2), center=true); // inner curve
            translate([0,0,-d]) cube([d*4,d*4,d*2],center=true); // lower cutoff
        }
        // and two legs
        translate([0,+d/2 - (thick/2),-h/2]) cube([width,thick,h],center=true); // leg 1
        translate([0,-d/2 + (thick/2),-h/2]) cube([width,thick,h],center=true); // leg 2
    }
}

module core_zipties() {
    translate([39,0,-2]) ziptie_arch(d=9);  // ziptie 1
    translate([-39,0,-2]) ziptie_arch(d=9); // ziptie 2
}

module necklace_cap() {
    difference() {
        union() {
            rotate([0,90,90]) linear_extrude(height=20, scale=1.2) circle(d=6.4); // conical outer face
            // translate([0,2,0]) scale([1.,1,1]) sphere(d=10); //
        }
        translate([0,-1,0]) rotate([0,90,90]) linear_extrude(height=32, scale=1.2) circle(d=5); // conical hollow
        translate([0,-2,0]) scale([1.2,0.6,0.45]) sphere(d=16); //
        difference() {
            union() {
                rotate([0,0,0])  translate([0,7,-10]) letter("妖",20,5);
                rotate([0,180,0])  translate([0,15,-10]) letter("姬",20,5);
            }
            rotate([0,90,90]) linear_extrude(height=20, scale=1.2) circle(d=5.8); // conical outer face
        }
    }
}

module holder_stand_wing() {
    // create a wing curve from two intersecting cylinders
   translate([25,4,35]) scale([1.3,1.0,0.8]) rotate([90,0,0]) linear_extrude(height=8) {
       difference() {
           circle(d=120, $fn=90); // conical outer face
           translate([0,7,0]) circle(d=118, $fn=90); // conical outer face
           translate([-100,0,0]) square([200,200]); // conical outer face
           translate([-20,-100,0]) square([200,200]); // conical outer face
       }
   }
   // feathers
   rx = 47; ry = 36;
   for(i=[0:16]) {
       a = i * 6;
       x = -sin(a)*rx - 5  - sin(a/3)*5; 
       y = -cos(a)*ry + 24 - sin(a/3+180)*15; ;
       translate([x,0,y]) {
            rotate([0,+a*1.4+110,0]) scale([0.9 - a/400,0.18,0.36 - i / 90]) sphere(d=20, $fn=18); // low res sphere for feather shape
       }
   }
}

module holder_stand(build_support=false) {
   translate([-0,0,-26]) linear_extrude(height=25, scale=0.5) circle(d=16, $fn=90); // top cone
   translate([-0,0,-30]) linear_extrude(height=6, scale=0.2) circle(d=50, $fn=90); // base upper cone
   translate([-0,0,-33]) linear_extrude(height=3, scale=0.5/0.47) circle(d=47, $fn=90); // base under cone
   translate([-0,0,-4]) scale([1.0,1.0,1.0]) sphere(d=12); // pommel
   holder_stand_wing();
   rotate([0,0,180]) holder_stand_wing();
   // feather build support
   if(build_support) difference() {
       translate([0,0,2]) cube([110,0.8,70],center=true);
       translate([0,5,45]) scale([1.2,0.8,1.0]) rotate([90,0,0]) linear_extrude(height=10) circle(d=100); // outer face
       translate([-90,0,-40]) rotate([0,70,0]) cube([130,1,100],center=true);
       translate([90,0,-40]) rotate([0,-70,0]) cube([130,1,100],center=true);
   }
   difference() {
       union() {
           // upper cradle points
           translate([-48,1,30]) rotate([0,-45,0]) cube([10,7,7],center=true);
           translate([+48,1,30]) rotate([0,+45,0]) cube([10,7,7],center=true);
           // lower cradle points
           translate([-30,1,12]) rotate([0,-60,0]) cube([12,7,6],center=true);
           translate([+30,1,12]) rotate([0,+60,0]) cube([12,7,6],center=true);
       }
       // amulet face shape
       translate([0,1,45]) rotate([90,0,0]) {
           scale(0.7) amulet_face();
           component_battery_space();
       }
   }
}

module amulet_body() {
    // take the pockets out of the amulet
    union() {
        difference() {
            amulet_etched();
            union() {         
                translate([-20.2,34,0]) cylinder(d=3, h=10, center=true); // attachment hole 1
                translate([20.2,34,0]) cylinder(d=3, h=10, center=true);  // attachment hole 2
                core_zipties();
                translate([19.5,-20,-16.6]) ziptie_arch(d=41);  // battery ziptie 1
                translate([-19.5,-20,-16.6]) ziptie_arch(d=41); // battery ziptie 2
                component_pockets();
            }
        
        }
        translate([-20,33,0.8]) attachment_ring();
        translate([20,33,0.8]) attachment_ring();
    }
}

module display_assembly() {
    amulet_body();
    amulet_crossbeams();    
    color("brown") eye_core();
    color("green") middle_deck();
    translate([-20,34,1]) rotate([0,0,16]) necklace_cap();
    translate([20,34,1]) rotate([0,0,-16]) necklace_cap();    
    translate([-0,-45,0]) rotate([-90,0,0])holder_stand();
}

module display_components(lid_angle=50,servo_angle=10) {
    eye_lids(lid_angle,servo_angle=servo_angle);
    // translate([0,-22,-1]) component_battery();
    translate([0,-20,-6]) rotate([0,90,0]) color("blue") cylinder(d=19,h=65,center=true); // battery round
    translate([20,18,0]) rotate([180,0,-200]) component_esp8266();
    translate([-24,0,0]) rotate([0,0,0])  component_servo(angle=lid_angle+servo_angle);
    translate([24,0,0]) rotate([0,0,180]) component_servo(angle=lid_angle+servo_angle);
    translate([0,0,03]) component_neopixel();
    color("black")  translate([-20,18,0]) rotate([0,0,110]) cube([12,30,1],center=true); // battery charger

}


module print_1() {
    rotate([180,0,0]) necklace_cap();
}


/* utility functions */

function mix_linear(v,i1,i2,o1,o2) =
    ( v < i1)
    ? o1
    : (v>i2)
        ? o2
        : (v-i1)/(i2-i1) * (o2-o1) + o1;

function mix(v,i1,i2,o1,o2) =
    ( i1<i2 ) 
    ? mix_linear(v,i1,i2,o1,o2) 
    : mix_linear(v,i2,i1,o2,o1);

function clamp_linear(v,c1,c2) =
    ( v < c1)
    ? c1
    : (v>c2)
        ? c2
        : v;

function clamp(v,c1,c2) =
    ( i1<i2 ) 
    ? clamp_linear(v,c1,c2) 
    : clamp_linear(v,c2,c1);

