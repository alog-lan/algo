#ifndef TOKEN_H
#define TOKEN_H

#include <stdint.h>

typedef enum {
    TOKEN_EOF = 0,
    TOKEN_ERROR,

    // Kalit so'zlar (Uzbek Keywords)
    TOKEN_AMAL,       // amal (funksiya)
    TOKEN_QAYTAR,     // qaytar (return)
    TOKEN_AGAR,       // agar (if)
    TOKEN_AKS,        // aks (else)
    TOKEN_TOKI,       // toki (while)
    TOKEN_UCHUN,      // uchun (for)
    TOKEN_CHIQAR,     // chiqar (print)
    TOKEN_KIRIT,      // kirit (input)
    TOKEN_TOXTAT,     // toxtat / to'xtat (break)
    TOKEN_DAVOM,      // davom (continue)
    TOKEN_TUZILMA,    // tuzilma (struct)
    TOKEN_YANGI,      // yangi (new / instantiate)
    TOKEN_UZUNLIK,    // uzunlik (len)
    TOKEN_VA,         // va (&&)
    TOKEN_YOKI,       // yoki (||)
    TOKEN_INKOR,      // inkor / ! (not)
    TOKEN_YUKLA,      // yukla (import)

    // Literallar va identifikatorlar
    TOKEN_IDENT,      // o'zgaruvchi, funksiya, tuzilma nomi
    TOKEN_INT,        // 64-bitli butun son
    TOKEN_STRING,     // matn ("...")

    // Belgilar va qavslar
    TOKEN_LBRACE,     // {
    TOKEN_RBRACE,     // }
    TOKEN_LPAREN,     // (
    TOKEN_RPAREN,     // )
    TOKEN_LBRACKET,   // [
    TOKEN_RBRACKET,   // ]
    TOKEN_COMMA,      // ,
    TOKEN_SEMICOLON,  // ;
    TOKEN_DOT,        // .
    TOKEN_COLON,      // :

    // Operatorlar
    TOKEN_ASSIGN,     // =
    TOKEN_PLUS,       // +
    TOKEN_MINUS,      // -
    TOKEN_STAR,       // *
    TOKEN_SLASH,      // /
    TOKEN_PERCENT,    // %

    // Taqqoslash operatorlari
    TOKEN_EQ,         // ==
    TOKEN_NEQ,        // !=
    TOKEN_LT,         // <
    TOKEN_LTE,        // <=
    TOKEN_GT,         // >
    TOKEN_GTE,        // >=
} TokenType;

typedef struct {
    TokenType type;
    char *text;
    int length;
    int64_t int_val;
    int line;
    int col;
} Token;

const char *token_type_to_string(TokenType type);

#endif // TOKEN_H
