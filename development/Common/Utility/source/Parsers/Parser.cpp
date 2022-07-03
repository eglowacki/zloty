#include "Parsers/Parser.h"
//#include "Parsers/exprtk.hpp"

//typedef exprtk::symbol_table<double> symbol_table_t;
//typedef exprtk::expression<double> expression_t;
//typedef exprtk::parser<double> parser_t;
//typedef exprtk::parser_error::type error_t;

////std::string expression_str = "z := 2 [sin(x * pi)^3*3 + cos(pi / y)^4*4] % (2*3/3*2x + 3*4/4*3y)";
//std::string expression_str = "pi";

//double x = 1.1;
//double y = 2.2;
//double z = 3.3;

//symbol_table_t symbol_table;
//symbol_table.add_constants();
//symbol_table.add_variable("x", x);
//symbol_table.add_variable("y", y);
//symbol_table.add_variable("z", z);

//expression_t expression;
//expression.register_symbol_table(symbol_table);

//parser_t parser;

//if (!parser.compile(expression_str, expression))
//{
//    printf("Error: %s\tExpression: %s\n",
//        parser.error().c_str(),
//        expression_str.c_str());

//    for (std::size_t i = 0; i < parser.error_count(); ++i)
//    {
//        const error_t error = parser.get_error(i);
//        printf("Error: %02d Position: %02d Type: [%s] Msg: %s Expr: %s\n",
//            static_cast<int>(i),
//            static_cast<int>(error.token.position),
//            exprtk::parser_error::to_str(error.mode).c_str(),
//            error.diagnostic.c_str(),
//            expression_str.c_str());
//    }

//    return 1;
//}

//double result = expression.value();
//result;



yaget::math::Parser::Parser()
{

}


yaget::math::Parser::~Parser()
{

}
