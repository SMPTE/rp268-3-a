#!/usr/bin/perl

# list of formats
$desc[0] = "CbYCr";  $chroma[0] = "444"; $planar[0] = 0;
$desc[1] = "CbYCrA"; $chroma[1] = "444"; $planar[1] = 0;
$desc[2] = "CbYCrY"; $chroma[2] = "422"; $planar[2] = 0;
$desc[3] = "CbYACrYA"; $chroma[3] = "422"; $planar[3] = 0;
$desc[4] = "YCbCr";  $chroma[4] = "422"; $planar[4] = 1;
$desc[5] = "YCbCrA"; $chroma[5] = "422"; $planar[5] = 1;
$desc[6] = "CYY";    $chroma[6] = "420"; $planar[6] = 0;
$desc[7] = "CYAYA";  $chroma[7] = "420"; $planar[7] = 0;
$desc[8] = "YCbCr";  $chroma[8] = "420"; $planar[8] = 1;
$desc[9] = "YCbCrA"; $chroma[9] = "420"; $planar[9] = 1;
$desc[10] = "BGR";   $chroma[10] = "444"; $planar[10] = 0;
$desc[11] = "BGR";   $chroma[11] = "444"; $planar[11] = 1;
$desc[12] = "BGRA";   $chroma[12] = "444"; $planar[12] = 0;
$desc[13] = "BGRA";   $chroma[13] = "444"; $planar[13] = 1;
$desc[14] = "ARGB";   $chroma[14] = "444"; $planar[14] = 0;
$desc[15] = "ARGB";   $chroma[15] = "444"; $planar[15] = 1;
$desc[16] = "RGB";   $chroma[16] = "444"; $planar[16] = 0;
$desc[17] = "RGB";   $chroma[17] = "444"; $planar[17] = 1;
$desc[18] = "RGBA";   $chroma[18] = "444"; $planar[18] = 0;
$desc[19] = "RGBA";   $chroma[19] = "444"; $planar[19] = 1;
$desc[20] = "ABGR";   $chroma[20] = "444"; $planar[20] = 0;
$desc[21] = "ABGR";   $chroma[21] = "444"; $planar[21] = 1;

$rng_ycbcr[0] = 0; $tf_ycbcr[0] = "PQ"; 
$rng_ycbcr[1] = 0; $tf_ycbcr[1] = "BT709";

$rng_rgb[0] = 1; $tf_rgb[0] = "PQ";
$rng_rgb[1] = 0; $tf_rgb[1] = "PQ";
$rng_rgb[2] = 0; $tf_rgb[2] = "HLG";

$w[0] = 1920; $h[0] = 1080;
$w[1] = 3840; $h[1] = 2160;
$w[2] = 4096; $h[2] = 2160;
$w[3] = 8192; $h[3] = 4320;

$desc_idx = 0;
$ycbcr_idx = 0;
$rgb_idx = 0;
$size_idx = 0;
for ($byte_order = 0; $byte_order < 2; ++$byte_order)
{
  for ($direction = 0; $direction < 2; ++$direction)
  {
    for ($bit_depth = 10; $bit_depth <= 12; $bit_depth += 2)
    {
       for ($packing = 0; $packing < 3; ++$packing)
       {
         for ($encoding = 0; $encoding < 2; ++ $encoding)
         {
           if (index($desc[$desc_idx], "G") != -1)
           {
              $rng = $rng_rgb[$rgb_idx];
              $tf = $tf_rgb[$rgb_idx];
              $rgb_idx = ($rgb_idx + 1) % 3;
           } else {
              $rng = $rng_ycbcr[$ycbcr_idx];
              $tf = $tf_ycbcr[$ycbcr_idx];
              $ycbcr_idx = ($ycbcr_idx + 1) % 2;
           }
           $range_txt = $rng ? "fr" : "lr";
           $bo_txt = $byte_order ? "msbf" : "lsbf";
           $dir_txt = $direction ? "l2r" : "r2l";
           $pk_txt = ($packing == 0) ? "packed" : (($packing == 1) ? "ma" : "mb");
           $rle_txt = $encoding ? "rle" : "norle"; 
           $fn = "ctest_" . $bit_depth . "bpc" . $range_txt . "_" . $desc[$desc_idx] . $chroma[$desc_idx];
           if ($planar[$desc_idx]) { $fn = $fn . "p"; }
           $fn = "${fn}_${bo_txt}_${dir_txt}_${pk_txt}_${rle_txt}.dpx"; 
           print "generate_color_test_pattern -o ${fn} -tf ${tf} -corder ";
           print $desc[$desc_idx];
           print " -usefr ${rng} -bpc ${bit_depth} -planar ";
           print $planar[$desc_idx];
           print " -chroma ";
           print $chroma[$desc_idx];
           print " -w " . $w[$size_idx] . " -h " . $h[$size_idx];
           print " -dmd ${dir_txt} -order ${bo_txt} -packing ${pk_txt}";
           print " -encoding ${encoding}\n";

           $desc_idx = ($desc_idx + 1) % 22;
           $size_idx = ($size_idx + 1) % 4;
         }
       }
     }
   }
}
