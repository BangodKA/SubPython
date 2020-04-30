#define NumOperation(op, opfunc)\
{{op, Int, Int}, std::shared_ptr<MathOperation>(new opfunc<int, int>)},\
{{op, Int, Real}, std::shared_ptr<MathOperation>(new opfunc<int, double>)},\
{{op, Real, Int}, std::shared_ptr<MathOperation>(new opfunc<double, int>)},\
{{op, Real, Real}, std::shared_ptr<MathOperation>(new opfunc<double, double>)},\
{{op, Logic, Real}, std::shared_ptr<MathOperation>(new opfunc<bool, double>)},\
{{op, Real, Logic}, std::shared_ptr<MathOperation>(new opfunc<double, bool>)},\
{{op, Logic, Int}, std::shared_ptr<MathOperation>(new opfunc<bool, int>)},\
{{op, Int, Logic}, std::shared_ptr<MathOperation>(new opfunc<int, bool>)},\
{{op, Logic, Logic}, std::shared_ptr<MathOperation>(new opfunc<bool, bool>)},

#define CompOperation(op, opfunc)\
{{op, Int, Int}, std::shared_ptr<MathOperation>(new opfunc<int, int>)},\
{{op, Int, Real}, std::shared_ptr<MathOperation>(new opfunc<int, double>)},\
{{op, Real, Int}, std::shared_ptr<MathOperation>(new opfunc<double, int>)},\
{{op, Real, Real}, std::shared_ptr<MathOperation>(new opfunc<double, double>)},\
{{op, Logic, Real}, std::shared_ptr<MathOperation>(new opfunc<bool, double>)},\
{{op, Real, Logic}, std::shared_ptr<MathOperation>(new opfunc<double, bool>)},\
{{op, Logic, Int}, std::shared_ptr<MathOperation>(new opfunc<bool, int>)},\
{{op, Int, Logic}, std::shared_ptr<MathOperation>(new opfunc<int, bool>)},\
{{op, Logic, Logic}, std::shared_ptr<MathOperation>(new opfunc<bool, bool>)},\
{{op, Str, Str}, std::shared_ptr<MathOperation>(new opfunc<std::string, std::string>)},

#define LogicOperation(op, opfunc)\
{{op, Int, Int}, std::shared_ptr<MathOperation>(new opfunc<int, int>)},\
{{op, Int, Real}, std::shared_ptr<MathOperation>(new opfunc<int, double>)},\
{{op, Real, Int}, std::shared_ptr<MathOperation>(new opfunc<double, int>)},\
{{op, Real, Real}, std::shared_ptr<MathOperation>(new opfunc<double, double>)},\
{{op, Logic, Real}, std::shared_ptr<MathOperation>(new opfunc<bool, double>)},\
{{op, Real, Logic}, std::shared_ptr<MathOperation>(new opfunc<double, bool>)},\
{{op, Logic, Int}, std::shared_ptr<MathOperation>(new opfunc<bool, int>)},\
{{op, Int, Logic}, std::shared_ptr<MathOperation>(new opfunc<int, bool>)},\
{{op, Logic, Logic}, std::shared_ptr<MathOperation>(new opfunc<bool, bool>)},
// template<typename T1, typename T2>
// StackValue PlusOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
//     return StackValue(static_cast<T1>(op1.Get()) + static_cast<T2>(op2.Get()));
// }

// template<typename T1, typename T2>
// StackValue MinusOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
//     return StackValue(static_cast<T1>(op1.Get()) - static_cast<T2>(op2.Get()));
// }

// template<typename T1, typename T2>
// StackValue MulOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
//     return StackValue(static_cast<T1>(op1.Get()) * static_cast<T2>(op2.Get()));
// }

// template<typename T1, typename T2>
// StackValue MulStrLOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
// 	std::string new_str = "";
// 	std::string old_str = static_cast<T2>(op2.Get());

// 	for (int i = 0; i < static_cast<T1>(op1.Get()); ++i) {
// 		new_str += old_str;
// 	}
//     return StackValue(new_str);
// }

// template<typename T1, typename T2>
// StackValue MulStrROperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
// 	std::string new_str = "";
// 	std::string old_str = static_cast<T1>(op1.Get());

// 	for (int i = 0; i < static_cast<T2>(op2.Get()); ++i) {
// 		new_str += old_str;
// 	}
//     return StackValue(new_str);
// }

// template<typename T1, typename T2>
// StackValue DivOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
//     return StackValue(static_cast<T1>(op1.Get()) / static_cast<T2>(op2.Get()));
// }

// template<typename T1, typename T2>
// StackValue ModOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
//     return StackValue(static_cast<T1>(op1.Get()) % static_cast<T2>(op2.Get()));
// }

// template<typename T1, typename T2>
// StackValue LessOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
//     return StackValue(static_cast<T1>(op1.Get()) < static_cast<T2>(op2.Get()));
// }

// template<typename T1, typename T2>
// StackValue LessEqOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
//     return StackValue(static_cast<T1>(op1.Get()) <= static_cast<T2>(op2.Get()));
// }

// template<typename T1, typename T2>
// StackValue GreaterOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
//     return StackValue(static_cast<T1>(op1.Get()) > static_cast<T2>(op2.Get()));
// }

// template<typename T1, typename T2>
// StackValue GreaterEqOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
//     return StackValue(static_cast<T1>(op1.Get()) >= static_cast<T2>(op2.Get()));
// }

// template<typename T1, typename T2>
// StackValue EqualOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
//     return StackValue(static_cast<T1>(op1.Get()) == static_cast<T2>(op2.Get()));
// }

// template<typename T1, typename T2>
// StackValue EqualStrOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
//     return StackValue(false);
// }

// template<typename T1, typename T2>
// StackValue NotEqualOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
//     return StackValue(static_cast<T1>(op1.Get()) != static_cast<T2>(op2.Get()));
// }

static const std::map<BinaryKey, std::shared_ptr<MathOperation>> kMathBinaries {
    {{Lexeme::Or, Int, Int}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Int, Real}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Real, Int}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Real, Real}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Logic, Real}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Real, Logic}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Logic, Int}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Int, Logic}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Logic, Logic}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Str, Str}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Logic, Str}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Int, Str}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Real, Str}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Str, Logic}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Str, Real}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Str, Int}, std::shared_ptr<MathOperation>(new OrOperation)},

    {{Lexeme::And, Int, Int}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Int, Real}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Real, Int}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Real, Real}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Logic, Real}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Real, Logic}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Logic, Int}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Int, Logic}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Logic, Logic}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Str, Str}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Logic, Str}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Int, Str}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Real, Str}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Str, Logic}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Str, Real}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Str, Int}, std::shared_ptr<MathOperation>(new AndOperation)},

    {{Lexeme::Add, Str, Str}, std::shared_ptr<MathOperation>(new PlusOperation<std::string, std::string>)},
	{{Lexeme::Mul, Str, Int}, std::shared_ptr<MathOperation>(new MulStrROperation<std::string, int>)},
	{{Lexeme::Mul, Int, Str}, std::shared_ptr<MathOperation>(new MulStrLOperation<int, std::string>)},

    {{Lexeme::Equal, Str, Int}, std::shared_ptr<MathOperation>(new EqualStrOperation<std::string, int>)},
	{{Lexeme::Equal, Int, Str}, std::shared_ptr<MathOperation>(new EqualStrOperation<int, std::string>)},
    {{Lexeme::Equal, Str, Real}, std::shared_ptr<MathOperation>(new EqualStrOperation<std::string, double>)},
	{{Lexeme::Equal, Real, Str}, std::shared_ptr<MathOperation>(new EqualStrOperation<double, std::string>)},
    {{Lexeme::Equal, Str, Logic}, std::shared_ptr<MathOperation>(new EqualStrOperation<std::string, bool>)},
	{{Lexeme::Equal, Logic, Str}, std::shared_ptr<MathOperation>(new EqualStrOperation<bool, std::string>)},

	NumOperation(Lexeme::Add, PlusOperation)
	NumOperation(Lexeme::Sub, MinusOperation)
	NumOperation(Lexeme::Mul, MulOperation)
	NumOperation(Lexeme::Div, DivOperation)

	{{Lexeme::Mod, Int, Int}, std::shared_ptr<MathOperation>(new ModOperation<int, int>)},
	{{Lexeme::Mod, Logic, Int}, std::shared_ptr<MathOperation>(new ModOperation<bool, int>)},
	{{Lexeme::Mod, Int, Logic}, std::shared_ptr<MathOperation>(new ModOperation<int, bool>)},
	{{Lexeme::Mod, Logic, Logic}, std::shared_ptr<MathOperation>(new ModOperation<bool, bool>)},

	CompOperation(Lexeme::Less, LessOperation)
	CompOperation(Lexeme::Greater, GreaterOperation)
	CompOperation(Lexeme::LessEq, LessEqOperation)
	CompOperation(Lexeme::GreaterEq, GreaterEqOperation)
	CompOperation(Lexeme::Equal, EqualOperation)
  	CompOperation(Lexeme::NotEqual, NotEqualOperation)
};