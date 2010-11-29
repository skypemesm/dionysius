#! /usr/bin/perl

#***************************************** AUTHOR : SASWAT MOHANTY <smohanty [at] cs.tamu.edu> ***********************************************


use List::Util qw(shuffle sum max);
# ---- function to compute probability distribution

sub prob_dist
{


	$nbins = 50; #### NUMBER OF BINS...

	(@thisout) = @_;
	$maxval = max @thisout; 
	$maxindex = scalar @thisout;
 
	#---- calculate frequencies----
	$inter = int(($maxval / $nbins) + 0.99); 
	@freq = (0) x $nbins;
	$back_inter = $inter;
	for ($i = 0; $i <= $maxindex; $i++)
	{

		if ($thisout[$i]){
		 $val = int ($thisout[$i]/$inter);
		 $freq[$val]++;
		}
	}

	$sum = sum @freq;
	
	# calculate probabilities
	for ($i = 0; $i < scalar @freq; $i++)
	{
		$freq[$i] /= $sum;	
	}

	return @freq;
}


@files = <.\./sizes*.txt>;

foreach $file(@files)
{
	print OUT "******************************************************\n\n";	
	print $file."\n";

	open SIZES,"<",$file or die $!;
	@output = <SIZES>;
	chomp @output;
	close SIZES;

	$_ = $file;
	@filename = /(.*)\./ig;
	$filename = $filename[0];
	$filename = $filename.".results";


	open OUT, ">>", $filename or die $!;

	print OUT $file."\n\n";

	$i=0;
	@inp=();@out=();

	foreach $thisout(@output)
	{
		#print $thisout.":\n";
		($inp[$i],$out[$i] )= split(/\t/,$thisout);
		#print $inp[$i]."\n";
		$i++;
	}

	#print "@inp"."\n\n";
	#print "@out"."\n\n";

	@in_freq = prob_dist(@inp);
	@out_freq = prob_dist(@out);

	#print OUT "\n\nDISTRIBUTIONS:\n----------------------------------\n\n";
	#print OUT "INPUT\n"."@in_freq"."\n\n";
	#print OUT "OUTPUT\n"."@out_freq"."\n\n";

	#print OUT "******************************************************\n\n";


	#calculate RE
	$re = 0; $sre = 0; $chi = 0; $bd=0; $ir = 0;

	for($i = 0; $i< scalar @out_freq; $i++)
	{
		if ($in_freq[$i] != 0 && $out_freq[$i] != 0)  # for craigs RNG
		{
			#print $in_freq[$i].":".$out_freq[$i]."::".($in_freq[$i] * log ($in_freq[$i]/$out_freq[$i]) / log(2) )."\n";
			$re += ($in_freq[$i] * log ($in_freq[$i]/$out_freq[$i]) / log(2) );
			$sre += (($in_freq[$i] * log ($in_freq[$i]/$out_freq[$i]) / log(2) ) 
						+ ($out_freq[$i] * log ($out_freq[$i]/$in_freq[$i]) / log(2)) ); 
			
			$chi += (($out_freq[$i] - $in_freq[$i])^2)/$in_freq[$i] ;
			$bd +=  sqrt($out_freq[$i] * $in_freq[$i]) ;
		
			$ir += ( ($out_freq[$i] * log ($out_freq[$i]/(0.5*($in_freq[$i] + $out_freq[$i]) ))) 
					+ ($in_freq[$i] * log ($in_freq[$i]/(0.5*($in_freq[$i] + $out_freq[$i]))) )  ) / log(2); 


		}
	}

	#print OUT "RESULTS:\n----------------------------------\n\n";
	print OUT "RELATIVE ENTROPY:\t\t".$re."\n";
	print OUT "Symmetric RELATIVE ENTROPY:\t\t".$sre."\n";
	#print OUT "Chi-Square Distance:\t\t".$chi."\n";
	print OUT "Bhattacharya Distance:\t\t".$bd."\n";
	print OUT "Information Radius:\t\t".$ir."\n\n";

	close OUT;
 
}


exit 0;

