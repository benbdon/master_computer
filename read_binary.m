close all;
clear;
clc;

%fileID = fopen('store_image_test_007.bin');
fileID = fopen('test.png');
A = uint8(fread(fileID,[104,175],'char'))

%Convert the matrix into an image.
K = mat2gray(A, [0, 255]);

%Display the original image and the result of the conversion.
figure
imshow(transpose(K))

whos A
fclose(fileID);