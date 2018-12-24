#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <set>


#include <CodeSource.h>
#include <CodeObject.h>
#include <CFGFactory.h>


#include <lib/binutils/VMAInterval.hpp>


#include "CudaCFGFactory.hpp"
#include "CudaFunction.hpp"
#include "CudaBlock.hpp"
#include "CudaCodeSource.hpp"
#include "CFGParser.hpp"
#include "GraphReader.hpp"
#include "ReadCubinCFG.hpp"

using namespace Dyninst;
using namespace ParseAPI;
using namespace SymtabAPI;
using namespace InstructionAPI;


static bool
test_nvdisasm() 
{
  // check whether nvdisasm works
  int retval = system("nvdisasm > /dev/null") == 0;
  if (!retval) {
    std::cout << "WARNING: nvdisasm is not available on your path to analyze control flow in NVIDIA CUBINs" << std::endl;
  }
  return retval;
}


static bool
dumpCubin
(
 const std::string &cubin, 
 ElfFile *elfFile 
) 
{
  int retval = false;
  int fd = open(cubin.c_str(), O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
  if (fd != -1) {
    int len = elfFile->getLength();
    retval = write(fd, elfFile->getMemoryOriginal(), len) == len; 
    close(fd);
  }
  return retval;
}


// Iterate all the functions in the symbol table.
// Parse the ones that can be dumped by nvdisasm in sequence;
// construct a dummy block for others
static void
parseDotCFG
(
 const std::string &dot_filename, 
 const std::string &cubin,
 int cuda_arch,
 Dyninst::SymtabAPI::Symtab *the_symtab,
 std::vector<CudaParse::Function *> &functions
) 
{
  CudaParse::CFGParser cfg_parser;
  // Step 1: parse all function symbols
  std::vector<Symbol *> symbols;
  the_symtab->getAllSymbols(symbols);
  // store functions that are not parsed by nvdisasm
  std::vector<Symbol *> unparsable_function_symbols;
  // remove functions that share the same names
  std::map<std::string, CudaParse::Function *> function_map;
  // test valid symbols
  for (auto *symbol : symbols) {
    if (symbol->getType() == Dyninst::SymtabAPI::Symbol::ST_FUNCTION) {
      auto index = symbol->getIndex();
      const std::string cmd = "nvdisasm -fun " +
        std::to_string(index) + " -cfg -poff " + cubin + " > " + dot_filename;
      if (system(cmd.c_str()) == 0) {
        // only parse valid symbols
        CudaParse::GraphReader graph_reader(dot_filename);
        CudaParse::Graph graph;
        std::vector<CudaParse::Function *> funcs;
        graph_reader.read(graph);
        cfg_parser.parse(graph, funcs);
        // local functions inside a global function cannot be independently parsed
        for (auto *func : funcs) {
          if (function_map.find(func->name) == function_map.end()) {
            function_map[func->name] = func;
          }
        }
      } else {
        unparsable_function_symbols.push_back(symbol);
        std::cout << "WARNING: unable to parse function: " << symbol->getMangledName() << std::endl;
      }
    }
  }

  for (auto &iter : function_map) {
    functions.push_back(iter.second);
  }

  // Step 2: relocate functions
  for (auto *symbol : symbols) {
    for (auto *function : functions) {
      if (function->name == symbol->getMangledName()) {
        auto begin_offset = function->blocks[0]->begin_offset;
        for (auto *block : function->blocks) {
          for (auto *inst : block->insts) {
            inst->offset = (inst->offset - begin_offset) + symbol->getOffset();
          }
          block->address = block->insts[0]->offset;
        }
        function->blocks[0]->address = symbol->getOffset();
        function->address = symbol->getOffset();
      }
    }
  }

  // Step 3: add unparsable functions
  // rename function and block ids
  size_t max_block_id = 0;
  size_t max_function_id = 0;
  for (auto *function : functions) {
    function->id = max_function_id++;
    for (auto *block : function->blocks) {
      block->id = max_block_id++;
    }
  }
  // for functions that cannot be parsed
  for (auto *symbol : unparsable_function_symbols) {
    auto function_name = symbol->getMangledName();
    auto *function = new CudaParse::Function(max_function_id++, std::move(function_name));
    function->address = symbol->getOffset();
    auto block_name = symbol->getMangledName() + "_0";
    auto *block = new CudaParse::Block(max_block_id++, std::move(block_name));
    block->begin_offset = 0;
    block->address = symbol->getOffset();
    int len = cuda_arch >= 70 ? 16 : 8;
    // add dummy insts
    for (size_t i = block->address; i < block->address + symbol->getSize(); i += len) {
      block->insts.push_back(new CudaParse::Inst(i));
    }
    function->blocks.push_back(block);
    functions.push_back(function);
  }

  // step4: add compensate blocks that only contains nop instructions
  for (auto *symbol : symbols) {
    for (auto *function : functions) {
      if (function->name == symbol->getMangledName() && symbol->getSize() > 0) {
        int len = cuda_arch >= 70 ? 16 : 8;
        int function_size = function->blocks.back()->insts.back()->offset + len - function->address;
        int symbol_size = symbol->getSize();
        if (function_size < symbol_size) {
          auto *block = new CudaParse::Block(max_block_id, ".L_" + std::to_string(max_block_id));
          max_block_id++;
          while (function_size < symbol_size) {
            block->insts.push_back(new CudaParse::Inst(function_size + function->address));
            function_size += len;
          } 
          if (function->blocks.size() > 0) {
            auto *last_block = function->blocks.back();
            last_block->targets.push_back(
              new CudaParse::Target(last_block->insts.back(), block, CudaParse::TargetType::FALLTHROUGH));
          }
          function->blocks.push_back(block);
        }
      }
    }
  }

  // parse function calls
  cfg_parser.parse_calls(functions);
}


bool
readCubinCFG
(
 ElfFile *elfFile,
 Dyninst::SymtabAPI::Symtab *the_symtab, 
 Dyninst::ParseAPI::CodeSource **code_src, 
 Dyninst::ParseAPI::CodeObject **code_obj
) 
{
  static bool nvdisasm_usable = test_nvdisasm();
  bool dump_cubin_success = false;

  if (nvdisasm_usable) {
    std::string filename = "tmp";
    std::string cubin = filename;
    std::string dot = filename + ".dot";

    dump_cubin_success = dumpCubin(cubin, elfFile);
    if (!dump_cubin_success) {
      std::cout << "WARNING: unable to write a cubin to the file system to analyze its CFG" << std::endl; 
    } else {
      std::vector<CudaParse::Function *> functions;
      parseDotCFG(dot, cubin, elfFile->getArch(), the_symtab, functions);
      CFGFactory *cfg_fact = new CudaCFGFactory(functions);
      *code_src = new CudaCodeSource(functions, the_symtab); 
      *code_obj = new CodeObject(*code_src, cfg_fact);
      (*code_obj)->parse();
      return true;
    }
  }

  *code_src = new SymtabCodeSource(the_symtab);
  *code_obj = new CodeObject(*code_src);

  return false;
}
