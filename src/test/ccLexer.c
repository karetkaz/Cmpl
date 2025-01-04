#include <cmpl.h>
#include <time.h>

#include "../printer.h"
#include "../compiler.h"
#include "../code.h"

/// callback executed after vm execution is finished
static vmError onHalt(nfcContext ctx) {
	printFmt(stdout, NULL, "value = %f\n", argF32(ctx, 0));
	return noError;
}

int main() {
	// initialize
	rtContext rt = rtInit(NULL, 64 << 20);
	ccContext cc = ccInit(rt, installBase, onHalt);

	clock_t time = clock();
	ccParse(cc, "cmplStd/lib/math/vector/Polynomial.cmpl", 1, NULL);
	// ccParse(cc, __FILE__, __LINE__, ".3 + 5");
	printFmt(stdout, NULL, "Compiler time: %.6f sec\n", (float64_t) (clock() - time) / CLOCKS_PER_SEC);

	if (rt->errors != 0) {
		error(rt, __FILE__, __LINE__, "Parser error: %d", rt->errors);
		return rtClose(rt);
	}

	for (astn ast = cc->tokNext; ast != NULL; ast = ast->next) {
		printFmt(stdout, NULL, "%?s:%?u:token: `%.t`\n", ast->file, ast->line, ast);
	}

	return rtClose(rt);
}
