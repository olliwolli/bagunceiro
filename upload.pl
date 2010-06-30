#!/usr/bin/perl -w
 
use strict;  
use CGI;  
use CGI::Carp qw ( fatalsToBrowser );  
use File::Basename;  

$CGI::POST_MAX = 1024 * 5000;  

my $upload_dir = "img/";  

my $url="img";
 
my $safe_filename_characters = "a-zA-Z0-9_.-";  

my $query = new CGI;  
my $filename = $query->param("file");  
 
if ( !$filename )  
{  
 print $query->header ( );  
 print "There was a problem uploading your photo (try a smaller file).";  
 exit;  
}  

my $upload_filehandle = $query->upload("file");  

do {  
	my ( $name, $path, $extension ) = fileparse ( $filename, '\..*' );  
	my $rand_hex = join "", map { unpack "H*", chr(rand(256)) } 1..16;
	$filename = $rand_hex . $extension;
 
} while (-e $filename);


open ( UPLOADFILE, ">$upload_dir/$filename" ) or die "$!";  
binmode UPLOADFILE;  
 
while ( <$upload_filehandle> )  
{print UPLOADFILE;}  
close UPLOADFILE;  
 
print $query->header ( );  
print <<END_HTML;  
<a href="$url/$filename">$url/$filename</a>
END_HTML
