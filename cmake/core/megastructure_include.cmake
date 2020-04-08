
cmake_minimum_required( VERSION 3.1...3.16 )

function( link_megastructure targetname )
	target_link_libraries( ${targetname} megastructurelib )
endfunction( link_megastructure )
