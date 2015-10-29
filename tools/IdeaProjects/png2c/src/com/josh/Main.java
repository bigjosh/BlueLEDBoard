package com.josh;

import javax.imageio.ImageIO;
import java.awt.*;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;

// Reads in in a PNG and emmits a C array for the BlueLEDBoard project
// The input PNG can be any size, it automatically gets sampled in the middle of each pixel zone
// I've been just screen snipping the images


public class Main {

    public static void p( String s ) {
        System.out.println(s);
    }


    public static void main(String[] args) throws IOException {
	// write your code here

        int targetX = 16;
        int targetY = 7;

        BufferedImage i = ImageIO.read(new File(args[0]));

        p("w=" + i.getWidth());
        p("h=" + i.getHeight());

        int dx = i.getWidth()/targetX;
        int dy = i.getHeight()/targetY;


        for( int x = 0; x<targetX ; x++ ) {

            System.out.print("0b0");


            for (int y = 0; y < targetY; y++) {

                int transformedX = (x * dx) + (dx/2);   // Sample from the center of the Cell
                int transformedY = ((targetY-1-y) * dy) + (dy/2);       // Read y in inverted order

                Color c= new Color(i.getRGB(transformedX, transformedY));

                i.setRGB(transformedX, transformedY, (Color.RED).getRGB() );

                //System.out.println( transformedX +","+transformedY+":"+ (c.getGreen()));

                // Put a red dot in the sample location just so we can maybe eyball it later
                System.out.print( c.getRed() <100 ? "1" : "0");

            }
            System.out.println(",");
        }

        // Save output to check smapling locations...

        ImageIO.write( i , "png" , new File("output.png"));

    }
}
