zcat $1 | awk '$0 ~ "mpirun" { if(np>0 && max!=0) printf("%d %g %g %d\n", np, max, std, f); for(i=1;i<=NF;i++) { if($i ~ "-np"){np=$(i+1)} if($i ~ "-m"){f=$(i+1)} } max=0} $1 ~ "^FIRST_AGREEMENT" { if($2>max) {max = $2; std=$5;}} END {if( max != 0 ) printf("%d %g %g %d\n", np, max, std, f)}'>$1.first
zcat $1 | awk '$0 ~ "mpirun" { if(np>0 && max!=0) printf("%d %g %g %d\n", np, max, std, f); for(i=1;i<=NF;i++) { if($i ~ "-np"){np=$(i+1)} if($i ~ "-m"){f=$(i+1)} } max=0} $1 ~ "^STABILIZE_AGREEMENT" { if($2>max) {max = $2; std=$5;}} END {if( max != 0 ) printf("%d %g %g %d\n", np, max, std, f)}'>$1.stab
zcat $1 | awk '$0 ~ "mpirun" { if(np>0 && max!=0) printf("%d %g %g %d\n", np, max, std, f); for(i=1;i<=NF;i++) { if($i ~ "-np"){np=$(i+1)} if($i ~ "-m"){f=$(i+1)} } max=0} $1 ~ "^AFTER_FAILURE" { if($2>max) {max = $2; std=$5;}} END {if( max != 0 ) printf("%d %g %g %d\n", np, max, std, f)}'>$1.after
zcat $1 | awk '$0 ~ "mpirun" { for(i=1;i<=NF;i++) if($i ~ "-npmin") { np=$(i+1); break; }; } $0 ~ "            4         1000" { printf("%d %g %g %g\n", np, $5, $3, $4) }'>$1.imb



