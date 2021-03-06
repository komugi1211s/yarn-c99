/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: yarn_spinner.proto */

#ifndef PROTOBUF_C_yarn_5fspinner_2eproto__INCLUDED
#define PROTOBUF_C_yarn_5fspinner_2eproto__INCLUDED

#include "protobuf-c.h"

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1003000
# error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1004000 < PROTOBUF_C_MIN_COMPILER_VERSION
# error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif


typedef struct Yarn__Program Yarn__Program;
typedef struct Yarn__Program__NodesEntry Yarn__Program__NodesEntry;
typedef struct Yarn__Program__InitialValuesEntry Yarn__Program__InitialValuesEntry;
typedef struct Yarn__Node Yarn__Node;
typedef struct Yarn__Node__LabelsEntry Yarn__Node__LabelsEntry;
typedef struct Yarn__Instruction Yarn__Instruction;
typedef struct Yarn__Operand Yarn__Operand;


/* --- enums --- */

/*
 * The type of instruction that this is.
 */
typedef enum _Yarn__Instruction__OpCode {
  /*
   * Jumps to a named position in the node.
   * opA = string: label name
   */
  YARN__INSTRUCTION__OP_CODE__JUMP_TO = 0,
  /*
   * Peeks a string from stack, and jumps to that named position in
   * the node. 
   * No operands.
   */
  YARN__INSTRUCTION__OP_CODE__JUMP = 1,
  /*
   * Delivers a string ID to the client.
   * opA = string: string ID
   */
  YARN__INSTRUCTION__OP_CODE__RUN_LINE = 2,
  /*
   * Delivers a command to the client.
   * opA = string: command text
   */
  YARN__INSTRUCTION__OP_CODE__RUN_COMMAND = 3,
  /*
   * Adds an entry to the option list (see ShowOptions).
   * - opA = string: string ID for option to add
   * - opB = string: destination to go to if this option is selected
   * - opC = number: number of expressions on the stack to insert
   *   into the line
   * - opD = bool: whether the option has a condition on it (in which
   *   case a value should be popped off the stack and used to signal
   *   the game that the option should be not available)
   */
  YARN__INSTRUCTION__OP_CODE__ADD_OPTION = 4,
  /*
   * Presents the current list of options to the client, then clears
   * the list. The most recently selected option will be on the top
   * of the stack when execution resumes.
   * No operands.
   */
  YARN__INSTRUCTION__OP_CODE__SHOW_OPTIONS = 5,
  /*
   * Pushes a string onto the stack.
   * opA = string: the string to push to the stack.
   */
  YARN__INSTRUCTION__OP_CODE__PUSH_STRING = 6,
  /*
   * Pushes a floating point number onto the stack.
   * opA = float: number to push to stack
   */
  YARN__INSTRUCTION__OP_CODE__PUSH_FLOAT = 7,
  /*
   * Pushes a boolean onto the stack.
   * opA = bool: the bool to push to stack
   */
  YARN__INSTRUCTION__OP_CODE__PUSH_BOOL = 8,
  /*
   * Pushes a null value onto the stack.
   * No operands.
   */
  YARN__INSTRUCTION__OP_CODE__PUSH_NULL = 9,
  /*
   * Jumps to the named position in the the node, if the top of the
   * stack is not null, zero or false.
   * opA = string: label name 
   */
  YARN__INSTRUCTION__OP_CODE__JUMP_IF_FALSE = 10,
  /*
   * Discards top of stack.
   * No operands.
   */
  YARN__INSTRUCTION__OP_CODE__POP = 11,
  /*
   * Calls a function in the client. Pops as many arguments as the
   * client indicates the function receives, and the result (if any)
   * is pushed to the stack.		
   * opA = string: name of the function
   */
  YARN__INSTRUCTION__OP_CODE__CALL_FUNC = 12,
  /*
   * Pushes the contents of a variable onto the stack.
   * opA = name of variable 
   */
  YARN__INSTRUCTION__OP_CODE__PUSH_VARIABLE = 13,
  /*
   * Stores the contents of the top of the stack in the named
   * variable. 
   * opA = name of variable
   */
  YARN__INSTRUCTION__OP_CODE__STORE_VARIABLE = 14,
  /*
   * Stops execution of the program.
   * No operands.
   */
  YARN__INSTRUCTION__OP_CODE__STOP = 15,
  /*
   * Pops a string off the top of the stack, and runs the node with
   * that name.
   * No operands.
   */
  YARN__INSTRUCTION__OP_CODE__RUN_NODE = 16
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(YARN__INSTRUCTION__OP_CODE)
} Yarn__Instruction__OpCode;

/* --- messages --- */

struct  Yarn__Program__NodesEntry
{
  ProtobufCMessage base;
  char *key;
  Yarn__Node *value;
};
#define YARN__PROGRAM__NODES_ENTRY__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&yarn__program__nodes_entry__descriptor) \
    , (char *)protobuf_c_empty_string, NULL }


struct  Yarn__Program__InitialValuesEntry
{
  ProtobufCMessage base;
  char *key;
  Yarn__Operand *value;
};
#define YARN__PROGRAM__INITIAL_VALUES_ENTRY__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&yarn__program__initial_values_entry__descriptor) \
    , (char *)protobuf_c_empty_string, NULL }


/*
 * A complete Yarn program.
 */
struct  Yarn__Program
{
  ProtobufCMessage base;
  /*
   * The name of the program.
   */
  char *name;
  /*
   * The collection of nodes in this program.
   */
  size_t n_nodes;
  Yarn__Program__NodesEntry **nodes;
  /*
   * The collection of initial values for variables; if a PUSH_VARIABLE
   * instruction is run, and the value is not found in the storage, this
   * value will be used
   */
  size_t n_initial_values;
  Yarn__Program__InitialValuesEntry **initial_values;
};
#define YARN__PROGRAM__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&yarn__program__descriptor) \
    , (char *)protobuf_c_empty_string, 0,NULL, 0,NULL }


struct  Yarn__Node__LabelsEntry
{
  ProtobufCMessage base;
  char *key;
  int32_t value;
};
#define YARN__NODE__LABELS_ENTRY__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&yarn__node__labels_entry__descriptor) \
    , (char *)protobuf_c_empty_string, 0 }


/*
 * A collection of instructions
 */
struct  Yarn__Node
{
  ProtobufCMessage base;
  /*
   * The name of this node.
   */
  char *name;
  /*
   * The list of instructions in this node.
   */
  size_t n_instructions;
  Yarn__Instruction **instructions;
  /*
   * A jump table, mapping the names of labels to positions in the
   * instructions list.
   */
  size_t n_labels;
  Yarn__Node__LabelsEntry **labels;
  /*
   * The tags associated with this node.
   */
  size_t n_tags;
  char **tags;
  /*
   * the entry in the program's string table that contains the original
   * text of this node; null if this is not available    
   */
  char *sourcetextstringid;
};
#define YARN__NODE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&yarn__node__descriptor) \
    , (char *)protobuf_c_empty_string, 0,NULL, 0,NULL, 0,NULL, (char *)protobuf_c_empty_string }


/*
 * A single Yarn instruction.
 */
struct  Yarn__Instruction
{
  ProtobufCMessage base;
  /*
   * The operation that this instruction will perform.
   */
  Yarn__Instruction__OpCode opcode;
  /*
   * The list of operands, if any, that this instruction uses.
   */
  size_t n_operands;
  Yarn__Operand **operands;
};
#define YARN__INSTRUCTION__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&yarn__instruction__descriptor) \
    , YARN__INSTRUCTION__OP_CODE__JUMP_TO, 0,NULL }


typedef enum {
  YARN__OPERAND__VALUE__NOT_SET = 0,
  YARN__OPERAND__VALUE_STRING_VALUE = 1,
  YARN__OPERAND__VALUE_BOOL_VALUE = 2,
  YARN__OPERAND__VALUE_FLOAT_VALUE = 3
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(YARN__OPERAND__VALUE__CASE)
} Yarn__Operand__ValueCase;

/*
 * A value used by an Instruction.
 */
struct  Yarn__Operand
{
  ProtobufCMessage base;
  Yarn__Operand__ValueCase value_case;
  union {
    /*
     * A string.
     */
    char *string_value;
    /*
     * A boolean (true or false).
     */
    protobuf_c_boolean bool_value;
    /*
     * A floating point number.
     */
    float float_value;
  };
};
#define YARN__OPERAND__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&yarn__operand__descriptor) \
    , YARN__OPERAND__VALUE__NOT_SET, {0} }


/* Yarn__Program__NodesEntry methods */
void   yarn__program__nodes_entry__init
                     (Yarn__Program__NodesEntry         *message);
/* Yarn__Program__InitialValuesEntry methods */
void   yarn__program__initial_values_entry__init
                     (Yarn__Program__InitialValuesEntry         *message);
/* Yarn__Program methods */
void   yarn__program__init
                     (Yarn__Program         *message);
size_t yarn__program__get_packed_size
                     (const Yarn__Program   *message);
size_t yarn__program__pack
                     (const Yarn__Program   *message,
                      uint8_t             *out);
size_t yarn__program__pack_to_buffer
                     (const Yarn__Program   *message,
                      ProtobufCBuffer     *buffer);
Yarn__Program *
       yarn__program__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   yarn__program__free_unpacked
                     (Yarn__Program *message,
                      ProtobufCAllocator *allocator);
/* Yarn__Node__LabelsEntry methods */
void   yarn__node__labels_entry__init
                     (Yarn__Node__LabelsEntry         *message);
/* Yarn__Node methods */
void   yarn__node__init
                     (Yarn__Node         *message);
size_t yarn__node__get_packed_size
                     (const Yarn__Node   *message);
size_t yarn__node__pack
                     (const Yarn__Node   *message,
                      uint8_t             *out);
size_t yarn__node__pack_to_buffer
                     (const Yarn__Node   *message,
                      ProtobufCBuffer     *buffer);
Yarn__Node *
       yarn__node__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   yarn__node__free_unpacked
                     (Yarn__Node *message,
                      ProtobufCAllocator *allocator);
/* Yarn__Instruction methods */
void   yarn__instruction__init
                     (Yarn__Instruction         *message);
size_t yarn__instruction__get_packed_size
                     (const Yarn__Instruction   *message);
size_t yarn__instruction__pack
                     (const Yarn__Instruction   *message,
                      uint8_t             *out);
size_t yarn__instruction__pack_to_buffer
                     (const Yarn__Instruction   *message,
                      ProtobufCBuffer     *buffer);
Yarn__Instruction *
       yarn__instruction__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   yarn__instruction__free_unpacked
                     (Yarn__Instruction *message,
                      ProtobufCAllocator *allocator);
/* Yarn__Operand methods */
void   yarn__operand__init
                     (Yarn__Operand         *message);
size_t yarn__operand__get_packed_size
                     (const Yarn__Operand   *message);
size_t yarn__operand__pack
                     (const Yarn__Operand   *message,
                      uint8_t             *out);
size_t yarn__operand__pack_to_buffer
                     (const Yarn__Operand   *message,
                      ProtobufCBuffer     *buffer);
Yarn__Operand *
       yarn__operand__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   yarn__operand__free_unpacked
                     (Yarn__Operand *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*Yarn__Program__NodesEntry_Closure)
                 (const Yarn__Program__NodesEntry *message,
                  void *closure_data);
typedef void (*Yarn__Program__InitialValuesEntry_Closure)
                 (const Yarn__Program__InitialValuesEntry *message,
                  void *closure_data);
typedef void (*Yarn__Program_Closure)
                 (const Yarn__Program *message,
                  void *closure_data);
typedef void (*Yarn__Node__LabelsEntry_Closure)
                 (const Yarn__Node__LabelsEntry *message,
                  void *closure_data);
typedef void (*Yarn__Node_Closure)
                 (const Yarn__Node *message,
                  void *closure_data);
typedef void (*Yarn__Instruction_Closure)
                 (const Yarn__Instruction *message,
                  void *closure_data);
typedef void (*Yarn__Operand_Closure)
                 (const Yarn__Operand *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCMessageDescriptor yarn__program__descriptor;
extern const ProtobufCMessageDescriptor yarn__program__nodes_entry__descriptor;
extern const ProtobufCMessageDescriptor yarn__program__initial_values_entry__descriptor;
extern const ProtobufCMessageDescriptor yarn__node__descriptor;
extern const ProtobufCMessageDescriptor yarn__node__labels_entry__descriptor;
extern const ProtobufCMessageDescriptor yarn__instruction__descriptor;
extern const ProtobufCEnumDescriptor    yarn__instruction__op_code__descriptor;
extern const ProtobufCMessageDescriptor yarn__operand__descriptor;

PROTOBUF_C__END_DECLS


#endif  /* PROTOBUF_C_yarn_5fspinner_2eproto__INCLUDED */
