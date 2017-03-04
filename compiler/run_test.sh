for file in `ls -d -1 $PWD/tests/uai/*.uai`
do
	outName=`echo $file | rev | cut -f 1 -d'/' | rev`
	outName=`echo $outName | cut -f 1 -d'.'`

	echo "### Compiling $file to $PWD/tests/$outName.bc"
	rm -rf $PWD/tests/$outName.bc
	./uai $file $PWD/tests/$outName.bc

	echo "# Testing generated bitecode"
	rm -rf $PWD/tests/$outName.ll
	llvm-dis $PWD/tests/$outName.bc
	diff $PWD/tests/bytecode/$outName.ll $PWD/tests/bytecode/$outName.ll
	if [ $? -ne 0 ]; then
	    echo "Generated unexpected bytecode"
	    exit
	fi

	echo "# Executing bitecode"
	rm -rf $PWD/tests/$outName.out
	lli $PWD/tests/$outName.bc >$PWD/tests/$outName.out
	# TODO(EDER): IMPLEMENT EXIT CODE ON MAIN
	#if [ $? -ne 0 ]; then
	#    echo "Execution fail"
	#fi

	echo "# Testing output produced"
	diff $PWD/tests/stdout/$outName.txt $PWD/tests/$outName.out
	if [ $? -ne 0 ]; then
	    echo "does not produce espected answer"
	    exit
	fi
	echo OK
done