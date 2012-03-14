#! /usr/bin/tclsh
package require Itcl
namespace import ::itcl::*

# Base data-structures for the interface we read in

proc new {typename args} {
  set obj [eval "$typename #auto $args"]
  return [$obj fqn]
}

#An Identifier stores the C++ and Tcl names for an item
class Identifier {
  variable cpp
  variable tcl
  constructor names {
    set cpp [lindex $names 0]
    if {[llength $names] > 1} {
      set tcl [lindex $names 1]
    } else {
      set tcl $cpp
    }
  }

  method inCpp {} {return $cpp}
  method inTcl {} {return $tcl}
  method fqn {} {return $this}
}

# Abstract class that encapsulates all code generating declarations
class Declaration {
  protected variable name
  protected variable isConst
  protected variable isPublic ;# if no, protected
  protected variable isStatic
  protected variable isVirtual
  protected variable isPureVirtual
  protected variable isInClassContext
  protected variable throwSpec
  public variable implfile

  constructor {nam inClassContext {memmods ""}} {
    set name [new Identifier $nam]
    set isConst no
    set isPublic yes
    set isStatic no
    set isVirtual no
    set isPureVirtual no
    set isInClassContext $inClassContext
    set throwSpec {}
    set implfile $::currentImplFile
    foreach mod $memmods {
      switch -glob $mod {
        const {set isConst yes}
        protected {set isPublic no}
        static {set isStatic yes}
        virtual {set isVirtual yes}
        purevirtual {set isVirtual yes; set isPureVirtual yes}
        {throw *} {set throwSpec $mod}
        noth {set throwSpec noth}
        default {error "Unknown member modifier $mod"}
      }
    }
  }
  method getName {} {return $name}
  method fqn {} {return $this}
  # The "glue" code is generally classes/functions/procs
  # that automate conversion and importation/exportation
  method generateC++Glue {}
  method generateTclGlue {}
  # The pre-declaration code lists function declarations needed
  # for the C++ dec code to work.
  # Defgult is an empty string.
  method generateC++PreDec {} {}
  # The declaration code is executed exactly one time
  # to put the glue into place
  method generateC++Dec {}
  method generateTclDec {} {}
  # Members need to define these as well. They declare
  # code to run in the constructor/destructor, respectively.
  method generateAuxConstructor {} {}
  method generateAuxDestructor {} {}
}

# Abstract type for type conversion
# All converters have a Tcl interpreter interp in their context
class Type {
  protected variable constness
  protected variable memoryModel ;#One of borrow, steal, yield, or copy
  protected variable mmstrict ;#True for STEAL and YIELD
  public variable cppCheckCode
  constructor {} {
    set constness no
    set memoryModel borrow
    set mmstrict no
    set cppCheckCode ""
  }

  method setModifiers {mods} {
    set memoryModel borrow
    set cppCheckCode {}
    for {set i 0} {$i < [llength $mods]} {incr i} {
      switch -glob [lindex $mods $i] {
        const {set constness yes}
        borrow {set memoryModel borrow}
        steal {
          set memoryModel steal
          set mmstrict no
        }
        STEAL {
          set memoryModel steal
          set mmstrict yes
        }
        yield {
          set memoryModel yield
          set mmstrict no
        }
        YIELD {
          set memoryModel yield
          set mmstrict yes
        }
        copy { set memoryModel copy }
        "check *" {
          set str [lindex $mods $i]
          set len [string length $str]
          set newCode [string range $str [string length "check "] [expr {$len-1}]]
          if {[string length $cppCheckCode]} {
            puts "WARNING: Replacing C++ validation code."
            puts "Old code:\n$cppCheckCode\n----\nNew code:\n$newCode\n"
          }
          set cppCheckCode $newCode
        }
        default {error "Unknown type modifier [lindex $mods $i]"}
      }
    }
  }
  method getC++Name {}
  # Returns the type used to declare a temporary reference.
  # The default is the same as getC++Name
  method getC++RefName {} {getC++Name}
  # Returns the type used to declare a temporary possible
  # pointer. Default returns the same as getC++Name.
  method getC++PtrName {} {getC++Name}
  # Returns a string to prefix to a type from getC++PtrName
  # to convert it back to getC++Name
  # Default is empty
  method getC++PtrDeref {} {return ""}
  # Called when a Tcl value passes into C++
  method generateC++ImportCode {objectIn valueOut}
  # Mostly same as above, but only do the ownership changes
  # This is called upon export errors; ie, this
  # MUST return code that undoes whatever
  # generateC++ExportCode had done. For example, the
  # copy memory model will delete the value copied
  # in the other method. This must NOT delete the
  # object itself, as that is done by independant code
  # Default is empty
  method generateC++ExportCodeUndo {objectIn} {}
  # Called when a C++ value passes into Tcl; the third argument specifies whether
  # any ownership change could take place (this is false for variables, for
  # example). This is also called on the old value of a variable when its
  # value is changed, with objectOut set to "" (the type must handle this
  # by not trying to create a Tcl_Obj).
  method generateC++ExportCode {valueIn objectOut {ownerCanChange yes}}
  # Mostly same as generateC++ExportCode, but only do ownership changes
  # See notes for generateC++ExportCodeUndo
  # Default is empty
  method generateC++ImportCodeUndo {valueIn} {}
  # Called when a C++ value passes into Tcl
  method generateTclImportCode {valueIn valueOut} {
    return "set $valueOut \$$valueIn\n"
  }

  method fqn {} {return $this}
}

# Built-in types
# Natives, like bool, are singletons per C++ type
class BoolType {
  inherit Type
  method getC++Name {} {return bool}
  method generateC++ImportCode {objectIn valueOut} {
    set tmpint [genC++Id]
    return "int $tmpint;
            int err = Tcl_GetBooleanFromObj(interp, $objectIn, (int*)&$tmpint);
            $valueOut=$tmpint;
            if (err == TCL_ERROR) {
              scriptError(Tcl_GetStringResult(interp));
            }"
  }
  method generateC++ExportCode {valueIn objectOut {occ yes}} {
    if {$objectOut == ""} {return ""}
    format {%s = Tcl_NewBooleanObj(%s);} $objectOut $valueIn
  }
}
BoolType boolean

# Handle int and unsigned (and short and such)
class IntType {
  inherit Type
  variable typename
  constructor {typ} { set typename $typ }
  method getC++Name {} {return $typename}
  method generateC++ImportCode {objectIn valueOut} {
    format {int tmp;
            int err = Tcl_GetIntFromObj(interp, %s, &tmp);
            if (err == TCL_ERROR)
              scriptError(Tcl_GetStringResult(interp));
            %s = (%s)tmp;} $objectIn $valueOut $typename
  }
  method generateC++ExportCode {valueIn objectOut {occ yes}} {
    if {[string length $objectOut]} {
      format {%s = Tcl_NewIntObj((int)%s);} $objectOut $valueIn
    } else { return "" }
  }
}
IntType int int
IntType unsigned unsigned
IntType Uint32 Uint32
IntType short short
IntType ushort {unsigned short}

class FloatType {
  inherit Type
  variable typename
  constructor {typ} { set typename $typ }
  method getC++Name {} {return $typename}
  method generateC++ImportCode {objectIn valueOut} {
    format {double tmp;
            int err = Tcl_GetDoubleFromObj(interp, %s, &tmp);
            if (err == TCL_ERROR)
              scriptError(Tcl_GetStringResult(interp));
            %s = (%s)tmp;} $objectIn $valueOut $typename
  }
  method generateC++ExportCode {valueIn objectOut {occ yes}} {
    if {[string length $objectOut]} {
      format {%s = Tcl_NewDoubleObj((double)%s);} $objectOut $valueIn
    } else {return ""}
  }
}
FloatType float float
FloatType double double

class CharType {
  inherit Type
  method getC++Name {} {return char}
  method generateC++ImportCode {objectIn valueOut} {
    format {
      if (Tcl_GetCharLength(%s) != 1)
          scriptError("Attempt to pass char of length other than 1");
      %s = Tcl_GetStringFromObj(%s, NULL)[0];
    } $objectIn $valueOut $objectIn
  }
  method generateC++ExportCode {valueIn objectOut {occ yes}} {
    if {[string length $objectOut]} {
      format {
        char str[2] = {%s, 0};
        %s = Tcl_NewStringObj(str, 1);
      } $valueIn $objectOut
    } else {return ""}
  }
  method generateTclImportCode {} { return "" }
}
CharType char

class CStrType {
  inherit Type
  method getC++Name {} {return {const char*}}
  method generateC++ImportCode {objectIn valueOut} {
    switch $memoryModel {
      borrow {
        format {
          static Tcl_DString dstr;
          static bool hasDstr=false;
          if (!hasDstr) {
            Tcl_DStringInit(&dstr);
            hasDstr=true;
          } else {
            Tcl_DStringSetLength(&dstr, 0);
          }
          int length;
          Tcl_UniChar* tuc = Tcl_GetUnicodeFromObj(%s, &length);
          %s = Tcl_UniCharToUtfDString(tuc, length, &dstr);
        } $objectIn $valueOut
      }
      copy {
        set original [genC++Id]
        set copied [genC++Id]
        set length [genC++Id]
        return \
        "int $length;
         static Tcl_DString dstr;
         static bool hasDstr=false;
         if (!hasDstr) {
           Tcl_DStringInit(&dstr);
           hasDstr=true;
         } else {
           Tcl_DStringSetLength(&dstr, 0);
         }
         Tcl_UniChar* tuc = Tcl_GetUnicodeFromObj($objectIn, &$length);
         const char* $original = Tcl_UniCharToUtfDString(tuc, $length, &dstr);
         char* $copied = new char\[$length+1\];
         strcpy($copied, $original);
         $valueOut=$copied;"
      }
      default { error "Unsupported memory model for const char*: $memoryModel" }
    }
  }
  method generateC++ExportCode {valueIn objectOut {occ yes}} {
    if {[string length $objectOut]} {
      format {
        static Tcl_DString dstr;
        static bool hasDstr=false;
        if (!hasDstr) {
          hasDstr = true;
          Tcl_DStringInit(&dstr);
        } else {
          Tcl_DStringSetLength(&dstr, 0);
        }
        Tcl_UniChar* tuc = Tcl_UtfToUniCharDString(%s? %s : "", -1, &dstr);
        %s = Tcl_NewUnicodeObj(tuc, -1);
      } $valueIn $valueIn $objectOut
    } else {return ""}
  }
}
CStrType cstr;

class StringType {
  inherit Type
  variable isReference
  constructor {isr} {if {$isr} {set isReference {&}} else {set isReference ""}}

  method getC++Name {} {
    format "%sstring%s" [
      if {$constness} {expr {"const "}} else {expr {""}}
    ] $isReference
  }
  method generateC++ImportCode {objectIn valueOut} {
    format {
      static Tcl_DString dstr;
      static bool hasDstr=false;
      if (!hasDstr) {
        Tcl_DStringInit(&dstr);
        hasDstr=true;
      } else {
        Tcl_DStringSetLength(&dstr, 0);
      }
      int length;
      Tcl_UniChar* tuc = Tcl_GetUnicodeFromObj(%s, &length);
      %s = Tcl_UniCharToUtfDString(tuc, length, &dstr);
    } $objectIn $valueOut
  }
  method generateC++ExportCode {valueIn objectOut {occ yes}} {
    if {[string length $objectOut]} {
      format {
        static Tcl_DString dstr;
        static bool hasDstr=false;
        if (!hasDstr) {
          hasDstr = true;
          Tcl_DStringInit(&dstr);
        } else {
          Tcl_DStringSetLength(&dstr, 0);
        }
        Tcl_UniChar* tuc = Tcl_UtfToUniCharDString(%s.c_str(), -1, &dstr);
        %s = Tcl_NewUnicodeObj(tuc, -1);
      } $valueIn $objectOut
    } else {return ""}
  }
}
StringType stdString no
StringType stdStringRef yes

# Pass Tcl_Obj* without conversion
class TclObjType {
  inherit Type
  method getC++Name {} { return "Tcl_Obj*" }
  method generateC++ImportCode {objectIn valueOut} {
    format {%s = %s;} $valueOut $objectIn
  }
  method generateC++ExportCode {valueIn objectOut {occ yes}} {
    if {[$objectOut != ""]} {
      format {%s = %s;} $objectOut $valueIn
    } else {return ""}
  }
}
TclObjType tclobj

# Pass interpreters between C++ and Tcl
class TclInterpType {
  inherit Type
  method getC++Name {} { return "Tcl_Interp*" }
  method generateC++ImportCode {objectIn valueOut} {
    return "
      $valueOut = Tcl_GetSlave(invokingInterpreter,
                               Tcl_GetStringFromObj($objectIn, NULL));
    "
  }
  method generateC++ExportCode {valueIn objectOut {occ yes}} {
    return "
      if (TCL_ERROR == Tcl_GetInterpPath(invokingInterpreter, $valueIn)) {
        scriptError(\"Couldn't find path to slave intepreter.\");
      }
      $objectOut = Tcl_GetObjResult(invokingInterpreter);
    "
  }
}
TclInterpType tclinterp

# Enumerated types
# An enumeration may be made "open"; in this case,
# unknown C++ values are mapped to the zeroth
# name
class EnumType {
  inherit Type
  variable cppType
  variable identifierList
  variable isOpen
  constructor {cppt prefix open args} {
    set identifierPairs $args
    set cppType $cppt
    set isOpen $open
    # Construct our list of identifiers
    set identifierList ""
    for {set i 0} {$i < [llength $identifierPairs]} {incr i} {
      set pair [lindex $identifierPairs $i]
      set cname [lindex $pair 0]
      if {[llength $pair] > 1} {
        set tclname [lindex $pair 1]
      } else {
        set tclname $cname
      }
      set tclname ${prefix}${tclname}
      lappend identifierList [Identifier #auto [list $cname $tclname]]
    }
  }

  method getC++Name {} { return $cppType }
  method generateC++ImportCode {objectIn valueOut} {
    set build [
      format {
        const char* tmp = Tcl_GetStringFromObj(%s, NULL);
        //Protect from buffer overflows in static error messages
        if (strlen(tmp) > 100) { scriptError("Enumeration value too long"); }
      } $objectIn
    ]
    set doneLabel [genC++Id done]
    for {set i 0} {$i < [llength $identifierList]} {incr i} {
      # MSVC++ cannot handle more than 128 if/else statements chained together.
      # Therefore, do some work for it by replacing else with goto doneLabel
      append build [
        format {
          if (0 == strcmp(tmp, "%s")) {%s=%s; goto %s;}
        } [[lindex $identifierList $i] inTcl] $valueOut [[lindex $identifierList $i] inCpp] $doneLabel
      ]
    }
    append build [
      format { {
        sprintf(staticError, "Unable to convert %%s to %s", tmp);
        scriptError(staticError);
      } } $cppType
    ]
    append build "\n$doneLabel:;"
    return $build
  }

  method generateC++ExportCode {valueIn objectOut {occ yes}} {
    if {$objectOut == ""} {return ""}
    set build [
      format {
        const char* tmp=NULL;
        switch (%s)
      } $valueIn
    ]
    append build "{\n"
    for {set i 0} {$i < [llength $identifierList]} {incr i} {
      append build [
        format {
          case %s: tmp="%s"; break;} \
          [[lindex $identifierList $i] inCpp] \
          [[lindex $identifierList $i] inTcl]
      ]
    }
    if {$isOpen} {
      append build [
        format {
          default:
          tmp = "%s"; break;} \
          [[lindex $identifierList 0] inTcl]
      ]
    } else {
      append build [
        format {
          default:
          cerr << "FATAL: Unable to convert enumeration %s value "
               << (int)%s << " (invalid!)!" << endl;
          ::exit(EXIT_PROGRAM_BUG);
        } $cppType $valueIn
      ]
    }
    append build "\n}\n"
    append build [
      format {
        %s = Tcl_NewStringObj(tmp, -1);
      } $objectOut
    ]
    return $build
  }
}

# Adapter for all C++ composite types
class CompositeType {
  inherit Type
  variable name
  # If foreign, we use the static type alone, not the dynamic type,
  # as I'm uncertain about the portability of RTTI and C structs.
  variable isForeign

  variable isImmediate
  # For the C++ symbols (foo*, &foo, *foo)
  variable pointer
  variable reference
  variable dereference

  constructor {nam isf isim} {
    set name [new Identifier $nam]
    set isForeign $isf
    set isImmediate $isim
    if {$isImmediate} {
      set pointer ""
      set reference "&"
      set dereference "*"
    } else {
      set pointer "*"
      set reference ""
      set dereference ""
    }
  }

  method getC++Name {} {
    if {$constness} {
      set constspec {const }
    } else {
      set constspec ""
    }
    return "$constspec[$name inCpp]$pointer"
  }
  method getC++RefName {} {
    if {$isImmediate} {
      return "[getC++Name]&"
    } else {
      return [getC++Name]
    }
  }
  method getC++PtrName {} {
    if {!$isImmediate} {
      getC++Name
    } else {
      return "[getC++Name]*"
    }
  }

  method getC++PtrDeref {} {
    return $dereference
  }

  method generateC++ImportCode {objectIn valueOut} {
    set segfaultHandler [genC++Id segmentation_fault]
    switch $memoryModel {
      borrow {
        set changeOwnership {}
      }
      steal {
        # C++ is importing, so it gets control
        # We do need to make sure it's not automatic, though
        # and if strict, make sure it's not already C++
        if {$mmstrict} {
          set strictHandler "scriptError(\"Double-import into C++\"); break;\n"
        } else {
          set strictHandler ""
        }
        set changeOwnership \
       "if (tmp) switch (tmp->ownStat) {
          case AObject::Cpp: $strictHandler
          case AObject::Tcl:
            tmp->ownStatBak=tmp->ownStat;
            tmp->ownStat=AObject::Cpp;
            //So undo works properly
            tmp->ownerBak.interpreter=tmp->owner.interpreter;
            break;
          case AObject::Container:
            scriptError(\"Change of ownership of automatic C++ value\");
            break;
        }"
      }
      yield {
        # Basically the opposite of steal
        # C++ is importing, so it loses control
        # We do need to make sure it's not automatic, though
        # and if strict, make sure it's not already Tcl
        if {$mmstrict} {
          set strictHandler "scriptError(\"Double-import out of C++\");break;\n"
        } else {
          set strictHandler ""
        }
        set changeOwnership \
       "if (tmp) switch (tmp->ownStat) {
          case AObject::Tcl:
            if (interp == tmp->owner.interpreter) {
              $strictHandler
            } //fall through
          case AObject::Cpp:
            tmp->ownStatBak=tmp->ownStat;
            tmp->ownStat=AObject::Tcl;
            tmp->ownerBak.interpreter=tmp->owner.interpreter;
            tmp->owner.interpreter=interp;
            break;
          case AObject::Container:
            scriptError(\"Change of ownership of automatic C++ value\");
            break;
        }"
      }
      copy {
        # Nice and simple
        # Assumes the presence of a copy constructor
        set changeOwnership "tmp = new [$name inCpp](*tmp);"
      }
    }
    set code "
      string name(Tcl_GetStringFromObj($objectIn, NULL));
      if (name != \"0\") {
        //Does it exist?
        InterpInfo* info=interpreters\[interp\];
        map<string,Export*>::iterator it=info->exportsByName.find(name);
        if (it == info->exportsByName.end()) {
          for (it=info->exportsByName.begin();
               it != info->exportsByName.end(); ++it) {
            cout << (*it).first << endl;
          }
          sprintf(staticError, \"Invalid export passed to C++: %s\",
                  name.c_str());
          scriptError(staticError);
        }
        Export* ex=(*it).second;
        //OK, is the type correct?
        if (ex->type->theType != typeid([$name inCpp])
        &&  0==ex->type->superclasses.count(&typeid([$name inCpp]))) {
          //Nope
          sprintf(staticError, \"Wrong type passed to C++ function; expected\"
                               \" [$name inCpp], \"
                               \"got %s\", ex->type->tclClassName.c_str());
          scriptError(staticError);
        }

        //All is well, transfer ownership now
        [$name inCpp]* tmp=([$name inCpp]*)ex->ptr;
        $changeOwnership
        $valueOut = tmp;
    } else "
    if {$isImmediate} {
      append code "{scriptError(\"Null pointer assigned to immediate\");}\n"
    } else {
      append code "$valueOut=NULL;\n"
    }
  }

  method generateC++ExportCode {valueInRaw objectOut {changeOwner yes}} {
    set valueIn "($reference$valueInRaw)"
    if {$changeOwner} {
      switch $memoryModel {
        borrow {set changeOwnership ""}
        steal {
          # Tcl gets ownership; even though it's probably meaningless,
          # we'll honour strictness
          if {$mmstrict} {
            # This is C++'s fault
            set strictHandler {
              cerr << "FATAL: Unexpected double-export, strictly steal to Tcl"
                   << endl;
              ::exit(EXIT_PROGRAM_BUG);
              break;
            }
          } else {
            set strictHandler ""
          }
          set changeOwnership \
         "if ($valueIn) switch ($valueIn->ownStat) {
            case AObject::Tcl:
              if (interp == $valueIn->owner.interpreter) {
                $strictHandler
              }
              //fall through
            case AObject::Cpp:
              $valueIn->ownStatBak=$valueIn->ownStat;
              $valueIn->ownerBak.interpreter = $valueIn->owner.interpreter;
              $valueIn->ownStat=AObject::Tcl;
              $valueIn->owner.interpreter=interp;
              break;
            case AObject::Container:
              cerr <<
\"FATAL: Attempt by C++ to give Tcl ownership of automatic value\" << endl;
              ::exit(EXIT_PROGRAM_BUG);
          }"
        }
        yield {
          # C++ gets ownership; even though it's probably meaningless,
          # we'll honour strictness
          if {$mmstrict} {
            # This is C++'s fault
            set strictHandler {
              cerr << "FATAL: Unexpected double-export, strictly yield from Tcl" << endl;
              ::exit(EXIT_PROGRAM_BUG);
              break;
            }
          } else {
            set strictHandler ""
          }
          set changeOwnership \
         "if ($valueIn) switch ($valueIn->ownStat) {
            case AObject::Cpp: $strictHandler
            case AObject::Tcl:
              $valueIn->ownStatBak=$valueIn->ownStat;
              $valueIn->ownStat=AObject::Cpp;
              //So undo works correctly
              $valueIn->ownerBak.interpreter=$valueIn->owner.interpreter;
              break;
            case AObject::Container:
              cerr <<
\"FATAL: Unexpected yielding of automatic value from Tcl to C++\" << endl;
              ::exit(EXIT_PROGRAM_BUG);
          }"
        }
      }
    } else {
      set changeOwnership ""
    }

    if {$objectOut == ""} {return $changeOwnership}

    if {$isForeign} {
      set getTypeInfo "typeid([$name inCpp])"
      set setContainedMode ""
    } else {
      global implicitConversion
      set getTypeInfo "typeid(*$valueIn)"
      if {$isImmediate && ![info exists implicitConversion([$name inCpp])]} {
        set setContainedMode "$valueIn->ownStat = AObject::Container;"
      } else {
        set setContainedMode ""
      }
    }

    return \
   "if (!$valueIn) {
      $objectOut=Tcl_NewStringObj(\"0\", 1);
    } else {
      InterpInfo* info=interpreters\[interp\];
      //Is it a valid export already?
      void* ptr=const_cast<void*>((void*)$valueIn);
      map<void*,Export*>::iterator it=info->exports.find(ptr);
      Export* ex;
      if (it == info->exports.end()) {
        //No, create new one
        [expr {$isForeign? "": "$valueIn->tclKnown=true;"}]
        ex=new Export;
        ex->ptr=ptr;
        ex->type=typeExports\[&$getTypeInfo\];
        ex->interp=interp;
        $setContainedMode

        //Create the new Tcl-side object with
        //  new Type {}
        Tcl_Obj* cmd\[3\] = {
          Tcl_NewStringObj(\"new\", 3),
          Tcl_NewStringObj(ex->type->tclClassName.c_str(),
                           ex->type->tclClassName.size()),
          Tcl_NewObj(),
        };
        for (unsigned i=0; i<lenof(cmd); ++i)
          Tcl_IncrRefCount(cmd\[i\]);
        int status=Tcl_EvalObjv(interp, lenof(cmd), cmd, TCL_EVAL_GLOBAL);
        for (unsigned i=0; i<lenof(cmd); ++i)
          Tcl_DecrRefCount(cmd\[i\]);
        if (status == TCL_ERROR) {
          sprintf(staticError, \"Error exporting C++ object to Tcl: %s\",
                  Tcl_GetStringResult(interp));
          scriptError(staticError);
        }

        //We can now get the name, and then have a fully-usable export
        ex->tclrep=Tcl_GetStringResult(interp);
        //Register
        info->exports\[(void*)$valueIn\]=ex;
        info->exportsByName\[ex->tclrep\]=ex;
      } else {
        //Yes, use directly
        ex = (*it).second;
      }

      //Ownership
      $changeOwnership

      //Done
      $objectOut=Tcl_NewStringObj(ex->tclrep.c_str(), ex->tclrep.size());
    }"
  }

  method generateC++ExportCodeUndo {value} {
    switch $memoryModel {
      borrow {return ""}
      yield -
      steal {
        return \
        "if ($value) {
          $value->ownStat=$value->ownStatBak;
          $value->owner.interpreter=$value->ownerBak.interpreter; }"
      }
      copy {
        # When copied, we just new'd one, so get rid of it now
        return "if ($value) delete $value;"
      }
    }
  }

  method generateC++ImportCodeUndo {value} {
    # All operations will be same as export undo, since
    # we use backup ownstats
    generateC++ExportCodeUndo $value
  }
}

# Access to std::pair<A,B>;
# the Tcl value is mapped to a list
class PairType {
  inherit Type
  variable lType
  variable rType
  variable lMods
  variable rMods

  constructor {l r lm rm} {
    set lType [resolveType $l]
    set rType [resolveType $r]
    set lMods $lm
    set rMods $rm
  }

  method getC++Name {} {
    format {pair<%s,%s>} \
      [$lType setModifiers $lMods; $lType getC++Name] \
      [$rType setModifiers $rMods; $rType getC++Name]
  }

  method generateC++ExportCode {valueIn objectOut} {
    return \
   "Tcl_Obj* l; {[$lType setModifiers $lMods; $lType generateC++ExportCode $valueIn.first l]};
    Tcl_Obj* r; {[$rType setModifiers $rMods; $rType generateC++ExportCode $valueIn.second r]};
    Tcl_Obj* objv\[2\]={l,r};
    $objectOut=Tcl_NewListObj(2, objv);\n"
  }

  method generateC++ImportCode {objectIn valueOut} {
    return \
   "int length;
    if (TCL_ERROR == Tcl_ListObjLength(interp, $objectIn, &length)) {
      scriptError(\"Non-list passed as pair\");
    }
    if (length != 2) {
      scriptError(\"List of non-2 length passed as pair\");
    }
    Tcl_Obj* l, *r;
    if (TCL_ERROR == Tcl_ListObjIndex(interp, $objectIn, 0, &l)
    ||  TCL_ERROR == Tcl_ListObjIbdex(interp, $objectIn, 1, &r)) {
      scriptError(\"Could not extract pair from list for some reason\");
    }
    {[$lType setModifiers $lMods; $lType generateC++ImportCode l $valueOut.first]};
    {[$rType setModifiers $rMods; $rType generateC++ImportCode r $valueOut.second]};\n"
  }
}

proc newPairType args {
  switch [llength $args] {
    2 {set l [lindex $args 0]; set lm ""; set r [lindex $args 1]; set rm ""}
    3 {set l [lindex $args 0]; set lm [lindex $args 1]; set r [lindex $args 2]; set rm ""}
    4 {
      foreach {ix var} {0 l 1 lm 2 r 3 rm} {set $var [lindex $args $ix]}
    }
    default {
      error "Wrong # args passed to newPairType"
    }
  }
  set p [new PairType $l $r $lm $rm]
  global knownTypes
  set knownTypes([$p getC++Name]) $p
}

# Proc to automatically generate guaranteed-unique C++ identifiers
set nextCppIdNum 0
proc genC++Id {{pfx "gen"}} {
  global nextCppIdNum
  incr nextCppIdNum
  return ${pfx}${nextCppIdNum}
}

# Setup array of types
set knownTypes(bool) boolean
set knownTypes(int) int
set knownTypes(unsigned) unsigned
set knownTypes(Uint32) Uint32
set knownTypes(short) short
set knownTypes(ushort) ushort
set knownTypes(float) float
set knownTypes(double) double
set knownTypes(char) char
set knownTypes(cstr) cstr
set knownTypes(string) stdString
set knownTypes(string&) stdStringRef
set knownTypes(Tcl_Obj*) tclobj
set knownTypes(Tcl_Interp*) tclinterp
# Not actually a valid type, but it simplifies code
set knownTypes(void) void

proc resolveType {typename} {
  global knownTypes
  global templates
  set leftAngleIx [string first < $typename]
  if {$leftAngleIx != -1} {
    set templateType [string range $typename 0 [expr {$leftAngleIx-1}]]
  }
  if {[info exists knownTypes($typename)]} {return $knownTypes($typename)} \
  elseif {$leftAngleIx != -1 && [info exists templates($templateType)]} {
    set parms [string range $typename [expr {$leftAngleIx+1}] [expr {[string last > $typename]-1}]]
    set parms [split $parms ,]
    eval "instantiateTemplate_$templateType $parms"
    if {[info exists knownTypes($typename)]} {return $knownTypes($typename)}
    error "instantiateTemplate_$templateType did not register its type!"
  }
  error "Unknown type \"$typename\""
}

set declarations {}

## Declaration class types

# Handles all variables and constants.
class Variable {
  inherit Declaration

  variable varType
  variable typeMods

  variable cppSetName
  variable cppGetName
  # If in class context, the refl functions are global
  # which simply call the class functions.
  variable cppSetRefl
  variable cppGetRefl
  variable tclTraceName

  constructor {id vt tmods isInClass {memmods ""}} {
    Declaration::constructor $id $isInClass $memmods
  } {
    set varType [resolveType $vt]
    set typeMods $tmods

    set cppGetName [genC++Id get]
    if {!$isConst} {set cppSetName [genC++Id set]}
    set tclTraceName [genC++Id accessor]
  }

  method generateC++PreDec {} {
    set code "int ${cppGetName}(ClientData,Tcl_Interp*,int,Tcl_Obj*const\[\]);"
    if {!$isConst} {
      append code \
        "int ${cppSetName}(ClientData,Tcl_Interp*,int,Tcl_Obj*const\[\]);"
    }
    if {!$isInClassContext} {
      return $code
    } else {
      return {}
    }
  }

  # The C++ glue code is one or two functions that get and set
  # the variable in question. If this is a member item,
  # the get/set functions take the object as an additional
  # argument
  method generateC++Glue {} {
    global currentClassName
    global currentClassType
    if {$isInClassContext} {set staticDec static} else {set staticDec ""}
    if {$isInClassContext && !$isStatic} {
      set argc 1
      global currentClass
      set howToAccess "parent->[$name inCpp]"
      $currentClassType setModifiers {}
      set getParent\
      "[$currentClassType getC++Name] parent; {
       [$currentClassType generateC++ImportCode {objv[0]} parent]
      } if (!parent) { scriptError(\"NULL this passed into C++\"); }"
    } else {
      set argc 0
      set howToAccess [$name inCpp]
      set getParent ""
    }

    $varType setModifiers $typeMods

    set getglue \
    "$staticDec int ${cppGetName} (ClientData, Tcl_Interp* interp, int objc, Tcl_Obj*const objv\[\]) throw() {
      SHIFT;
      #define scriptError(desc) { scriptingErrorMessage=desc; goto error; }
      if (objc != $argc) {
        Tcl_SetResult(interp, \"Invalid usage of internal glue function $cppGetName\", TCL_VOLATILE);
        return TCL_ERROR;
      }
      Tcl_Obj* objout;
      $getParent
      {[$varType generateC++ExportCode $howToAccess objout no]}
      Tcl_SetObjResult(interp, objout);
      return TCL_OK;

      error:
      Tcl_SetResult(interp, scriptingErrorMessage, TCL_VOLATILE);
      return TCL_ERROR;
      #undef scriptError
    }\n"
    if {$isConst} {set setglue {}} else { set setglue \
    "$staticDec int ${cppSetName} (ClientData, Tcl_Interp* interp, int objc, Tcl_Obj*const objv\[\]) throw() {
      SHIFT;
      #define scriptError(desc) { scriptingErrorMessage=desc; goto error; }
      if (objc != $argc+1) {
        Tcl_SetResult(interp, \"Invalid usage of internal glue function $cppSetName\", TCL_VOLATILE);
        return TCL_ERROR;
      }
      [$varType getC++PtrName] newVal;
      $getParent

      //Extract value
      {[$varType generateC++ImportCode objv\[$argc\] newVal]}

      //Any necessary checks
      [expr {[string length [$varType cget -cppCheckCode]]?
     "#define val newVal
      #define currVal $howToAccess
      {
        bool ok=true;
        {[$varType cget -cppCheckCode]}
        #undef currVal
        #undef val

        if (!ok) {
          //Undo any importing, then fail
          {[$varType generateC++ImportCodeUndo newVal]}
          Tcl_SetResult(interp, \"Attempt to set \\\"[$name inCpp]\\\" to invalid value.\", TCL_VOLATILE);
          return TCL_ERROR;
        }
      }" : ""}]

      //Transfer ownership of old value if necessary
      {[$varType generateC++ExportCode $howToAccess ""]}

      //Set
      $howToAccess = [$varType getC++PtrDeref]newVal;

      //Return successfully
      Tcl_SetObjResult(interp, objv\[$argc\]);
      return TCL_OK;

      error:
      Tcl_SetResult(interp, scriptingErrorMessage, TCL_VOLATILE);
      return TCL_ERROR;
      #undef scriptError
    }\n" }

    return $getglue$setglue
  }

  # We define a proc or method to attach to the trace
  method generateTclGlue {} {
    set thisRef ""
    set procspec proc
    set globalSpec "global [$name inTcl]"
    set vardecSpec ""
    if {$isInClassContext && !$isStatic} {
      set thisRef \$this
      set procspec method
      set globalSpec ""
      set vardecSpec "[expr {$isPublic? "public":"protected"}] variable [$name inTcl]\n"
    }
    if {!$isConst} {
      # Write handler forwards new value to C++
      set writeHandler "return \[{c++ $cppSetName} $thisRef \$[$name inTcl]\]"
    } else {
      # Writing triggers error
      set writeHandler "error \"Cannot write to [$name inTcl]\""
    }
    return "$vardecSpec $procspec $tclTraceName {ignored ignored op} {
             $globalSpec
             if {\$op == {read}} {
               set tmp \[{c++ $cppGetName} $thisRef\]
               [$varType generateTclImportCode tmp [$name inTcl]]
             } elseif {\$op == {write}} {
               $writeHandler
             } else { error \"Cannot unset C++ variable [$name inTcl]\" }
           }\n"
  }

  # C++ dec code declares the one or two commands to Tcl
  method generateC++Dec {} {
    set getdec \
    "Tcl_CreateObjCommand(interp, \"c++ ${cppGetName}\", ${cppGetName}, 0, NULL);"
    if {$isConst} {set setdec {}} else {set setdec \
    "\nTcl_CreateObjCommand(interp, \"c++ ${cppSetName}\", ${cppSetName}, 0, NULL);"}
    return "$getdec$setdec"
  }

  # Tcl dec code is blank for members, but declares a trace for globals
  method generateTclDec {} {
    if {$isInClassContext && !$isStatic} {return ""}
    return "set [$name inTcl] {}\ntrace add variable [$name inTcl] {read write unset} $tclTraceName"
  }

  # Members need to register/deregister traces
  method generateAuxConstructor {} {
    return "set [$name inTcl] {}
            trace add variable [$name inTcl] {read write unset} \"\$this $tclTraceName\""
  }
  method generateAuxDestructor {} {
    return "trace remove variable [$name inTcl] {read write unset} \"\$this $tclTraceName\""
  }
}

class Function {
  inherit Declaration

  variable retType
  variable retTypeMods
  protected variable argTypes
  protected variable argTypesMods

  # Non-virtual functions have a single C++-side trampoline
  # that calls the appropriate C++ function. Virtual functions have
  # an additional default caller that forces the call to go to
  # the implementation of the C++ class.
  variable mainTrampoline
  variable defaultTrampoline

  constructor {id rett arglist isInClass {memmods ""}} {
    Declaration::constructor $id $isInClass $memmods
  } {
    set retType [resolveType [lindex $rett 0]]
    set retTypeMods [lrange $rett 1 [llength $rett]]
    set argTypes {}
    set argTypesMods {}
    foreach argspec $arglist {
      set t ""
      set hasType no
      set mods {}
      foreach arg $argspec {
        if {!$hasType} {
          set t $arg
          set hasType yes
        } else {
          lappend mods $arg
        }
      }
      set t [resolveType $t]
      lappend argTypes $t
      lappend argTypesMods $mods
    }

    set mainTrampoline [genC++Id trampoline]
    set defaultTrampoline [genC++Id deftramp]
  }

  method generateC++PreDec {} {
    set code \
"int ${mainTrampoline}(ClientData,Tcl_Interp*,int,Tcl_Obj*const\[\])throw();"
    if {!$isInClassContext} {
      return $code
    } else {
      return {}
    }
  }

  # See notes below on generateC++Glue for implementation details.
  # This exists since the main and default trampolines are basically
  # the same.
  method generateTrampoline {isMain} {
    global currentClassName
    global currentClassType

    if {$isInClassContext && !$isStatic} {
      set firstProperArg 1
      set argc [expr {1+[llength $argTypes]}]
      $currentClassType setModifiers {}
      set declareParent "[$currentClassType getC++Name] parent=NULL;"
      set extractParent "{[$currentClassType generateC++ImportCode {objv[0]} parent]}
      if (!parent) { scriptError(\"NULL this passed into C++\"); }"
      set cancelParent  "if (parent) {[$currentClassType generateC++ImportCodeUndo parent]}"
    } else {
      set firstProperArg 0
      set argc [llength $argTypes]
      set declareParent ""
      set extractParent ""
      set cancelParent ""
    }

    set glue "\n#define scriptError(desc) { scriptingErrorMessage=desc; goto error; }\n"
    # We won't need two scopes, since we can actually return TCL_ERROR properly.
    # However, we DO need to catch any longjmp that may occur from our resulting
    # call to C++ (in case it jumps back into Tcl and something fails)
    append glue \
    "[if {$isInClassContext} {expr {{static}}}] int
     [if {$isMain} {expr {$mainTrampoline}} else {expr {$defaultTrampoline}}] (
     ClientData, Tcl_Interp* interp, int objc, Tcl_Obj*const objv\[\]) throw() {
       SHIFT;
       if (objc != $argc) {
         Tcl_SetResult(interp, \"Incorrect number of arguments passed to internal function\", TCL_VOLATILE);
         return TCL_ERROR;
       }
       invokingInterpreter=interp;
       ${declareParent}\n"
    for {set i 0} {$i < [llength $argTypes]} {incr i} {
      set at [lindex $argTypes $i]
      $at setModifiers [lindex $argTypesMods $i]
      append glue "[$at getC++PtrName] arg${i}; bool arg${i}Init=false;\n"
    }
    if {$retType != "void"} {
      $retType setModifiers $retTypeMods
      append glue "[$retType getC++Name] ret; Tcl_Obj* retTcl=NULL;\n"
    }

    # Now that everything's declared, setup the longjmp handler
    append glue "PUSH_TCL_ERROR_HANDLER(errorOccurred); if (errorOccurred) goto error;\n"

    # Begin importation
    append glue "$extractParent\n"
    for {set i 0} {$i < [llength $argTypes]} {incr i} {
      set at [lindex $argTypes $i]
      $at setModifiers [lindex $argTypesMods $i]
      append glue "{[$at generateC++ImportCode "objv\[[expr {$i+$firstProperArg}]\]" "arg${i}"]};\n"
      append glue "arg${i}Init=true;\n"
      if {[string length [$at cget -cppCheckCode]]} {
        append glue "{bool ok=true;
          #define val [$at getC++PtrDeref]arg${i}
          [$at cget -cppCheckCode]
          #undef val
          if (!ok) {
            scriptingErrorMessage = \"Unacceptable value to argument $i\";
            goto error;
          }
        }\n"
      }
    }

    # Success, make the call
    # If this is the default trampoline, we need to check to
    # see if we are Tcl-extended; if so, call the default
    # function; otherwise, do a normal virtual call
    set makeCall \
    "[if {$retType != "void"} {expr {{ret =}}}]
     [if {$isInClassContext && !$isStatic} {expr {{parent->}}}]
     [if {!$isMain} {expr {"${currentClassName}::"}}]
     [$name inCpp]("
    for {set i 0} {$i < [llength $argTypes]} {incr i} {
      set at [lindex $argTypes $i]
      $at setModifiers [lindex $argTypesMods $i]
      append makeCall  "[$at getC++PtrDeref]arg${i}[if {$i < [expr {[llength $argTypes]-1}]} {expr {{, }}}]"
    }
    append makeCall ");\n"
    if {!$isMain} {
      set makeCall "if (parent->tclExtended) $makeCall else [regsub ${currentClassName}:: $makeCall ""]"
    }
    append glue \
    "try {
      $makeCall
    } catch (exception& ex) {
      sprintf(staticError, \"%s: %s\", typeid(ex).name(), ex.what());
      scriptError(staticError);
    }\n"

    # Call successful, no issues
    # Now convert return value
    if {$retType != "void"} {
      $retType setModifiers $retTypeMods
      append glue "{[$retType generateC++ExportCode ret retTcl]}\n"
      # If we don't goto error (meaning we do get here), all was successful
      append glue "Tcl_SetObjResult(interp, retTcl);\n"
    }
    append glue \
     "POP_TCL_ERROR_HANDLER;
      return TCL_OK;\n"

    # Error handling
    append glue "error:
      POP_TCL_ERROR_HANDLER;
      double_error:
      #undef scriptError
      #define scriptError(msg) { \\
        cerr << \"Double-error; old message: \" << scriptingErrorMessage << \\
        \", new message: \" << msg << endl; \\
        scriptingErrorMessage = msg; goto double_error; \\
      }
      [if {$isInClassContext && !$isStatic} {expr {$cancelParent}}]\n"
    for {set i 0} {$i < [llength $argTypes]} {incr i} {
      set at [lindex $argTypes $i]
      $at setModifiers [lindex $argTypesMods $i]
      append glue "if (arg${i}Init) {arg${i}Init=false; [$at generateC++ImportCodeUndo "arg${i}"]}\n"
    }
    append glue "#undef scriptError\n"
    append glue "Tcl_SetResult(interp, scriptingErrorMessage, NULL); return TCL_ERROR; }"
    return $glue
  }


  # We implement non-member functions by creating a single glue ("trampoline")
  # C++ function that handles argument/return conversion, and a corresponding
  # Tcl proc that calls it. Non-virtual member functions work similarly, except
  # the Tcl-side uses a method. For virtual functions, the Tcl-side method calls
  # the default trampoline, and the standard trampoline actually forwards to
  # whatever Tcl method is in place (which may then call the default or be over-
  # ridden). Pure-virtual functions only have the main trampoline, which also
  # calls the Tcl-side method. However, the default Tcl-side method produces an
  # error since it isn't implemented.
  method generateC++Glue {} {
    global currentClassName
    global currentClassType

    # If we are in class context, we're currently with a C++ class extending
    # the class we're exporting. If virtual or pure-virtual, we must override
    # the exact function to call Tcl
    set glue "\n#define scriptError(desc) { scriptingErrorMessage=desc; goto error; }\n"

    if {$isVirtual} {
      append glue \
      "virtual
       [ if {$retType != "void"} { $retType setModifiers $retTypeMods; $retType getC++Name } \
         else {expr {{void}}}]
       [$name inCpp] ("
      for {set i 0} {$i < [llength $argTypes]} {incr i} {
        set at [lindex $argTypes $i]
        set atm [lindex $argTypesMods $i]
        $at setModifiers $atm
        append glue "[$at getC++Name] arg${i} [if {($i+1) < [llength $argTypes]} {expr {{,}}}]"
      }
      # We open a second scope here so we can goto out of it if necessary
      # This ensures that, eg, a string will be deallocated properly
      append glue ") [if {$isConst} {expr {{const}}}] $throwSpec { {
        //Forward to Tcl
        int status;
        Tcl_Interp* interp=tclExtended;
        Tcl_Obj* thisTcl=NULL;
        const char*const methodName = \"[$name inCpp]\";
        Tcl_Obj* meth=Tcl_NewStringObj(methodName, -1);
        Tcl_IncrRefCount(meth);\n"
        if {$retType != "void"} {
          append glue "
          Tcl_Obj* returnValueTcl=NULL;
          [$retType setModifiers $retTypeMods; $retType getC++PtrName] returnValue;\n"
        }
      # Declare all variables now, so that goto works
      for {set i 0} {$i < [llength $argTypes]} {incr i} {
        append glue "Tcl_Obj* arg${i}Tcl = NULL;\n"
      }
      # Continue
      $currentClassType setModifiers {}
      append glue "{[$currentClassType generateC++ExportCode this thisTcl]}
        Tcl_IncrRefCount(thisTcl);
      "
      for {set i 0} {$i < [llength $argTypes]} {incr i} {
        set at [lindex $argTypes $i]
        $at setModifiers [lindex $argTypesMods $i]
        append glue "{[$at generateC++ExportCode "arg${i}" "arg${i}Tcl"]}
          Tcl_IncrRefCount(arg${i}Tcl);
        "
      }
      # Put everything into an array, then call the function
      append glue " {
        Tcl_Obj* objv\[[expr {[llength $argTypes]+2}]\] = {
          thisTcl,
          meth,
          "
      for {set i 0} {$i < [llength $argTypes]} {incr i} {
        append glue "arg${i}Tcl, "
      }
      append glue "};
      status = Tcl_EvalObjv(interp, sizeof(objv)/sizeof(Tcl_Obj*), objv, TCL_EVAL_GLOBAL);
      }
      //Before checking status, decrement all ref counts.
      Tcl_DecrRefCount(thisTcl);
      thisTcl=NULL;
      Tcl_DecrRefCount(meth);
      meth=NULL;
      "
      for {set i 0} {$i < [llength $argTypes]} {incr i} {
        append glue "Tcl_DecrRefCount(arg${i}Tcl);\narg${i}Tcl=NULL;\n"
      }
      append glue "
      if (status == TCL_ERROR) {
        /* It's hard to reason about how the memory model should respond to this.
         * However, deexporting the arguments is probably a worse idea than
         * leaving them be.
         * So just get out.
         */
        scriptingErrorMessage = Tcl_GetStringResult(interp);
        goto long_jump;
      }

      //OK!\n"
      if {$retType != "void"} {
        append glue \
        "returnValueTcl = Tcl_GetObjResult(interp);
        {[$retType setModifiers $retTypeMods; $retType generateC++ImportCode returnValueTcl returnValue]}
        //Run any check code we may have
        { bool ok=true;
          #define val [$retType getC++PtrDeref]returnValue
          [$retType cget -cppCheckCode]
          #undef val
          if (!ok) {
            //Cancel import and fail
            [$retType generateC++ImportCodeUndo [$retType getC++PtrDeref]returnValue]
            scriptingErrorMessage = \"Unacceptable return value from Tcl override\";
            goto long_jump;
          }
        }
        return [$retType getC++PtrDeref]returnValue;\n"
      } else {
        append glue "return;\n"
      }

      append glue "
      error:
      //Decref anything existing
      if (thisTcl) Tcl_DecrRefCount(thisTcl);
      if (meth) Tcl_DecrRefCount(meth);
      "
      for {set i 0} {$i < [llength $argTypes]} {incr i} {
        set at [lindex $argTypes $i]
        $at setModifiers [lindex $argTypesMods $i]
        append glue "if (arg${i}Tcl) {
          //Deimport as well
          {[$at generateC++ImportCodeUndo "args${i}"]}
          Tcl_DecrRefCount(arg${i}Tcl);
        }\n"
      }
      #Fall off the end to the longjmp
      append glue "}
        long_jump:
        longjmp(currentTclErrorHandler, 1);
      }\n"
    } ;# End - if {$isVirtual}

    append glue [generateTrampoline yes]
    if {$isVirtual && !$isPureVirtual} {
      append glue [generateTrampoline no]
    }
    append glue "\n#undef scriptError\n"
    return $glue
  } ;# method generateC++Glue

  method generateTclGlue {} {
    set arglist ""
    for {set i 0} {$i < [llength $argTypes]} {incr i} {
      append arglist "a$i "
    }

    set access [expr {$isPublic? "" : "protected "}]
    if {!$isInClassContext || $isStatic} {
      set glue "proc "
    } else {
      set glue "${access}method "
    }
    append glue "{[$name inTcl]} { $arglist } {"
    set arglist ""
    for {set i 0} {$i < [llength $argTypes]} {incr i} {
      append arglist "\$a$i "
    }
    # In case the C++ side sources something, make
    # this function call invisible
    set bracedArglist {}
    foreach arg $arglist {
      set argf [lindex $arg 0]
      append bracedArglist "\"{$argf}\" "
    }
    append glue "set retpi \[uplevel 1 "
    if {!$isInClassContext || $isStatic} {
      append glue "\[list {c++ $mainTrampoline} $arglist\]"
    } elseif {!$isVirtual} {
      append glue "\[list {c++ $mainTrampoline} \$this $arglist\]"
    } elseif {!$isPureVirtual} {
      append glue "\[list {c++ $defaultTrampoline} \$this $arglist\]"
    } else { ;# Pure virtual
      append glue "error \"Call to pure-virtual function [$name inTcl]\""
    }
    append glue "\]\n"
    # Tcl importation
    if {$retType != "void"} {
      $retType setModifiers $retTypeMods
      append glue [$retType generateTclImportCode retpi ret]
      append glue "return \$ret\n"
    } else {
      append glue "return \$retpi\n"
    }
    append glue "}"
    return $glue
  }

  method generateC++Dec {} {
    set dec \
    "Tcl_CreateObjCommand(interp, \"c++ $mainTrampoline\", $mainTrampoline, 0, NULL);"
    if {$isVirtual && !$isPureVirtual} {
      append dec "\nTcl_CreateObjCommand(interp, \"c++ $defaultTrampoline\", $defaultTrampoline, 0, NULL);"
    }
    return $dec
  }
}

# Constructors are really just glorified functions.
class Constructor {
  inherit Function
  variable typePfx
  variable writeCppConstructor

  constructor {name class tp arguments {wcppc yes}} {
    Function::constructor "constructor$tp$name {c++ new $tp$class $name}" \
      "$class* [global currentClass;
                if {[$currentClass isForeign]} {
                  expr {{borrow}}
                } else {
                  expr {{steal}}}]" \
      "string string $arguments" yes static
  } {
    set typePfx $tp
    set writeCppConstructor $wcppc
  }

  method generateC++Glue {} {
    global currentClassName
    global currentExtendedClassName

    # Start i from 2 since 0 and 1 are names
    # for the c++ new function
    set argDecs ""
    for {set i 2} {$i < [llength $argTypes]} {incr i} {
      set at [lindex $argTypes $i]
      $at setModifiers [lindex $argTypesMods $i]
      append argDecs "[$at getC++Name] arg${i}"
      if {$i < [llength $argTypes]-1} {append argDecs ", "}
    }
    set argList ""
    for {set i 2} {$i < [llength $argTypes]} {incr i} {
      append argList "arg${i}"
      if {$i < [llength $argTypes]-1} {append argList ", "}
    }
    if {[llength $argTypes]-2 > 0} {
      set argComma ", "
    } else {
      set argComma ""
    }
    # Create forwarding constructor, then append static function,
    # than trampoline created by Function
    if {$writeCppConstructor} {
      set glue "${currentExtendedClassName}($argDecs) : ${currentClassName}($argList) {
        tclExtended=invokingInterpreter;
        tclKnown=true;
      }\n"
    } else {
      # Another constructor is present (this means that the class is non-abstract
      # extendable, and we are generating the extension constructor)
      set glue ""
    }
    set cppt [expr {$typePfx == "Tcl"? $::currentExtendedClassName : $::currentClassName}]
    append glue "static $currentClassName* [$name inCpp]
    (const string& name, const string& magicCookie$argComma $argDecs) {
      InterpInfo* info=interpreters\[invokingInterpreter\];
      //Validate magic cookie
      if (magicCookie != info->magicCookie) {
        scriptError(\"Call to c++ new with invalid magic cookie!\");
      }

      //Refuse if we enforce allocation and the interpreter knows too much
      if (info->enforceAllocationLimit && info->exports.size() > MAX_EXPORTS) {
        scriptError(\"Interpreter allocation limit exceeded\");
      }

      //Refuse duplicate allocation
      if (info->exportsByName\[name\]) {
        scriptError(\"Double allocation\");
      }
      $cppt* ret;
      ret=new ${cppt}($argList);
      if (ret) {
        ret->tclKnown = true;
        //The trampoline will take care of assigning ownership to Tcl, we
        //just need to create the export so it has the correct name
        Export* ex=new Export;
        info->exports\[(void*)ret\]=ex;
        info->exportsByName\[name\]=ex;
        ex->ptr = (void*)ret;
        ex->type = typeExports\[&typeid($cppt)\];
        ex->interp = invokingInterpreter;
        ex->tclrep=name;
      }

      return ret;
    }\n"
    append glue [Function::generateC++Glue]
    return $glue
  }

  method generateTclGlue {} {
    # Force the proc into the global namespace
    set    glue "namespace eval :: {\n"
    append glue [Function::generateTclGlue]
    append glue "}\n"
    return $glue
  }
}


proc extendedClassName id {
  set mapped [string map {
    < _leftangle_
    > _rightangle_
    * _asterix_
    & _ampersand_
    ( _leftparen_
    ) _rightparen_
    [ _leftbracket_
    ] _rightbracket_
    : _colon_
    " " _space_
    , _comma_
    . _dot_
  } "Tcl[$id inCpp]"]
  # C++ forbids consecutive underscores
  while {-1 != [string first __ $mapped]} {
    set ix [string first __ $mapped]
    set mapped [string replace $mapped $ix $ix+1 _u_]
  }
  return $mapped
}

set superclasses(AObject) {}
# Classes are implemented as follows. C++ extends whatever
# class we are exporting, and declares a static function
# within which is the global dec code. That function runs
# the dec code of all declarations in the class. The glue
# code of the class is the class itself, plus the glue
# generated by each declaration. Each constructor is
# given a name, and, when run, creates a new Tcl-owned
# instance of the /original/ C++ class. All constructors
# are also mirrored to Tcl<class>, which do the exact
# same except for creating an instance of the extending
# class (this is done ONLY for extendable classes). Each
# constructor takes the same arg list as the real C++
# counterpart, but also takes the name of the object
# as well as the magic cookie. These are named in
# Tcl as
#   c++ new CLASS CNAME
# For example,
#   c++ new Ship copy
# On the Tcl side, a class heirarchy is created that mirrors
# that of C++, with the exception that foreign types that
# don't extend anything extend AObject anyway. The constructor
# for Tcl AObject automatically handles passing the object
# name and magic cookie; each of the Tcl derived classes will
# handle passing the class name. Tcl-extendable classes determine
# which class to pass (ie, either Class or TclClass) based on
# the name of the constructor: If a * is prepended, the extendable
# version is constructed (with the * removed); otherwise, the
# standard version is created. There is no harm (other than a
# significant performance penalty) in creating a * version when
# not extending; however, if an extending class does not call
# the * constructor, C++ will not be aware of its extensions.
class Class {
  inherit Declaration

  variable classType ;#final, extendable, or foreign
  variable isAbstract
  variable superclasses
  variable innerDeclarations
  variable hasBody
  variable reflectFun

  constructor {ct nam supers bdy} {
    Declaration::constructor $nam no
  } {
    set isAbstract no
    set reflectFun [genC++Id classdec]
    switch $ct {
      final -
      extendable -
      coerced -
      foreign {set classType $ct}
      abstract-extendable {
        set classType extendable
        set isAbstract yes
      }
      default {error "Unknown class type: $ct"}
    }
    if {[string length $supers]} {
      set superclasses $supers
    } elseif {$classType != "foreign"} {
      set superclasses AObject
    } else {
      set superclasses ""
    }

    set typename [$name inCpp]
    if {$classType == "coerced"} {
      $name setCpp "ao<[$name inCpp]>"
    }

    # Register type
    set ptrT "$typename*"
    global knownTypes
    if {![info exists knownTypes($ptrT)]} {
      set knownTypes($ptrT) [
        new CompositeType [list [$name inCpp] [$name inTcl]] \
            [expr {$classType == {foreign}}] no]
      set knownTypes($typename) [
        new CompositeType [list [$name inCpp] [$name inTcl]] \
           [expr {$classType == {foreign}}] yes]
      set knownTypes([$name inTcl]*) $knownTypes($ptrT)
      set knownTypes([$name inTcl])  $knownTypes($typename)
    }


    set hasBody no
    if {0==[string length $bdy]} {return}
    set hasBody yes

    global declarations
    global currentClassName
    global currentClassNameTcl
    global currentClassType
    global currentExtendedClassName
    global currentClass
    set globalDecs $declarations
    set currentClassName [$name inCpp]
    set currentClassNameTcl [$name inTcl]
    set currentExtendedClassName [extendedClassName $name]
    set currentClass $this
    set currentClassType $knownTypes($ptrT)
    set declarations {}
    uplevel #0 { ;# Switch to member declarations
      proc class {args} { error "Nested classes not supported" }
      proc const {name type {typemods {}} {memmods {}}} {
        global declarations
        lappend declarations \
          [new Variable $name $type $typemods yes "const $memmods"]
      }
      proc var {name type {typemods {}} {memmods {}}} {
        global declarations
        lappend declarations [new Variable $name $type $typemods yes $memmods]
      }
      proc fun {returnTypeAndMods name {memmods {}} {args {}}} {
        global declarations
        lappend declarations \
          [new Function $name $returnTypeAndMods $args yes $memmods]
      }
      proc constructor {name {args {}}} {
        global declarations
        global currentClassNameTcl
        global currentClass
        set class $currentClassNameTcl
        if {![$currentClass isAbstract]} {
          lappend declarations [new Constructor $name $class {} $args]
        }
        if {[$currentClass isExtendable]} {
          lappend declarations \
            [new Constructor $name $class "Tcl" $args [$currentClass isAbstract]]
        }
      }
    }
    uplevel #0 $bdy
    uplevel #0 { ;# Switch back to toplevel
      rename constructor {}
      proc const {args} {eval "toplevelConst $args"}
      proc var {args} {eval "toplevelVar $args"}
      proc fun {args} {eval "toplevelFun $args"}
      proc class {args} {eval "toplevelClass $args"}
    }
    set innerDeclarations $declarations
    set declarations $globalDecs
  }

  method isExtendable {} {expr {$classType == "extendable"}}
  method isForeign {} {expr {$classType == "foreign"}}
  method isAbstract {} {return $isAbstract}

  method getSuperclasses class {
    set sups $::superclasses($class)
    set glue {}
    foreach sup $sups {
      append glue "ste->superclasses.insert(&typeid($sup));\n";
      append glue [getSuperclasses $sup]
    }
    return $glue
  }

  method generateC++PreDec {} {
    return "void ${reflectFun}(bool,Tcl_Interp*) throw();"
  }

  method generateC++Glue {} {
    if {!$hasBody} return
    global currentClassName
    global currentClassNameTcl
    global currentExtendedClassName
    global currentClassType
    global currentClass
    global knownTypes
    set currentClassName [$name inCpp]
    set currentClassNameTcl [$name inTcl]
    set currentExtendedClassName [extendedClassName $name]
    set currentClass $this
    set currentClassType $knownTypes([$name inCpp]*)
    set glue "class [extendedClassName $name] : public [$name inCpp] {
      public:\n"
    foreach dec $innerDeclarations {
      append glue "[$dec generateC++Glue]\n"
    }

    append glue "static void cppDecCode(bool safe,Tcl_Interp* interp) throw() {"
    foreach dec $innerDeclarations {
      append glue "[$dec generateC++Dec]\n"
    }
    # TypeExport info
    append glue "TypeExport* ste=new TypeExport(typeid([$name inCpp])),
                           * ete=new TypeExport(typeid([extendedClassName $name]));\n"
    append glue "ste->isAObject=ete->isAObject=[
      if {$classType == "foreign"} {expr {{false}}} else {expr {{true}}}];\n"
    append glue "ste->tclClassName=ete->tclClassName=\"[$name inTcl]\";\n"
    set ::superclasses([$name inCpp]) $superclasses
    append glue [getSuperclasses [$name inCpp]]
    append glue "ete->superclasses=ste->superclasses;\n"
    append glue "ete->superclasses.insert(&typeid([$name inCpp]));\n"
    append glue "typeExports\[&typeid([$name inCpp])\]=ste;\n"
    append glue "typeExports\[&typeid([extendedClassName $name])\]=ete;\n"
    append glue "}\n"

    append glue "};\n"
    append glue "void ${reflectFun}(bool safe, Tcl_Interp* interp) throw() {\n"
    append glue "  [extendedClassName $name]::cppDecCode(safe,interp);\n"
    append glue "}"
    return $glue
  }

  method generateC++Dec {} {
    if {!$hasBody} return
    return "${reflectFun}(safe, interp);"
  }

  method generateTclGlue {} {
    if {!$hasBody} return
    global currentClassName
    global currentClassNameTcl
    global currentClassType
    global currentExtendedClassName
    global currentClass
    global knownTypes
    set currentClassName [$name inCpp]
    set currentClassNameTcl [$name inTcl]
    set currentExtendedClassName [extendedClassName $name]
    set currentClass $this
    set currentClassType $knownTypes([$name inCpp]*)
    set glue "::itcl::class {[$name inTcl]} {\n"
    if {[llength $superclasses]} {
      set sups $superclasses
    } else {
      set sups AObject
    }
    foreach sup $sups {
      append glue "inherit $sup\n"
    }

    foreach dec $innerDeclarations {
      append glue "[$dec generateTclGlue]\n"
    }

    append glue "constructor {clazz cname arguments} {
      # Every class we extend (if it isn't just AObject) has
      # exactly one purpose in its constructor: Call the parent
      # constructor until we reach AObject. Therefore, to avoid
      # multiple instantiation, only call the first super
      [lindex $sups 0]::constructor "
    append glue "\[expr {\[string length \$cname\] && {*}==\[string index \$cname 0\]?
                 \"Tcl\$clazz\" : \$clazz}\] "
    append glue "\[expr {\[string length \$cname\] && {*}==\[string index \$cname 0\]?
                 \[string range \$cname 1 \[string length \$cname\]\] : \$cname}\] \$arguments"
    append glue "} {\n"
    foreach dec $innerDeclarations {
      append glue "[$dec generateAuxConstructor]\n"
    }
    append glue "}\n"
    append glue "destructor {\n"
    foreach dec $innerDeclarations {
      append glue "[$dec generateAuxDestructor]\n"
    }
    append glue "}\n"

    append glue "}\n"
    return $glue
  }

  method generateTclDec {} {
    if {!$hasBody} return
    global currentClassName
    global currentClassNameTcl
    global currentClassType
    global currentClass
    global knownTypes
    global currentExtendedClassName
    set currentExtendedClassName [extendedClassName $name]
    set currentClassName [$name inCpp]
    set currentClassNameTcl [$name inTcl]
    set currentClass $this
    set currentClassType $knownTypes([$name inCpp]*)
    set code "set glueClass([$name inTcl]) yes\n"
    foreach dec $innerDeclarations {
      append code "[$dec generateTclDec]\n"
    }
    return $code
  }
}

# An Unknowable represents some type that can't be
# extended and, for all we care, has no members or
# other contents.
# The actual "type" is represented by CompositeType
# like classes, but CompositeType always works for
# ANY export as long as the memory model is "borrow".
#
# Tcl-side still gets a class, but this class has
# no members, and exists solely to be a handle to
# the C++ pointer.
class Unknowable {
  inherit Declaration

  constructor {nam} {
    Declaration::constructor $nam no
  } {
    global knownTypes
    set knownTypes([$name inTcl]*) [new CompositeType $nam yes no]
  }

  method generateC++Glue {} {}
  method generateC++Dec {} {
    return "{
      TypeExport* ex=new TypeExport(typeid([$name inCpp]));
      ex->isAObject=false;
      ex->tclClassName = \"[$name inTcl]\";
      typeExports\[&typeid([$name inCpp])\]=ex;
    }"
  }

  method generateTclGlue {} {
    return "itcl::class [$name inTcl] {
      inherit AObject
      constructor {args} {
        AObject::constructor [$name inTcl] {} {}
      } {}
    }"
  }

  method generateTclDec {} {}
}

class ExcludeFromSafe {
  inherit Declaration
  variable innerDeclarations

  constructor {code} {
    Declaration::constructor "" no
  } {
    global declarations
    set declBak $declarations
    set declarations {}
    namespace eval :: $code
    set innerDeclarations $declarations
    set declarations $declBak
  }

  method generateC++PreDec {} {
    set ret ""
    foreach decl $innerDeclarations {
      append ret "[$decl generateC++PreDec]\n"
    }
    return $ret
  }

  method generateC++Glue {} {
    set ret ""
    foreach decl $innerDeclarations {
      append ret "[$decl generateC++Glue]\n"
    }
    return $ret
  }

  method generateC++Dec {} {
    set ret "if (!safe) {\n"
    foreach decl $innerDeclarations {
      append ret "[$decl generateC++Dec]\n"
    }
    append ret "}"
    return $ret
  }

  method generateTclGlue {} {
    set ret ""
    foreach decl $innerDeclarations {
      append ret "[$decl generateTclGlue]\n"
    }
    return $ret
  }

  method generateTclDec {} {
    set ret ""
    foreach decl $innerDeclarations {
      append ret "[$decl generateTclDec]\n"
    }
    return $ret
  }

  method generateAuxConstructor {} {
    set ret ""
    foreach decl $innerDeclarations {
      append ret "[$decl generateAuxConstructor]\n"
    }
    return $ret
  }

  method generateAuxDestructor {} {
    set ret ""
    foreach decl $innerDeclarations {
      append ret "[$decl generateAuxDestructor]\n"
    }
    return $ret
  }
}

## Forget "class" proc so we can use it
namespace forget class

## Toplevel procs

# Whether an enumeration is in a class or not doesn't really
# matter to us --- if it is not public, the extension class
# will just be the only thing that can handle the conversion
# (so it works normally as far as we care).
# We may sometimes want to create an enumeration out of an
# integer type, so allow specifying an alternate typename
# to register.
proc enum {namepair prefix args} {
  global knownTypes
  set name [new Identifier $namepair]
  # Use eval to expand args
  set conv [eval "new EnumType [$name inCpp] \$prefix no $args"]
  # All we need to do is register the type
  set knownTypes([$name inTcl]) $conv
  delete object $name
}

proc openenum {namepair prefix args} {
  global knownTypes
  set name [new Identifier $namepair]
  # Use eval to expand args
  set conv [eval "new EnumType [$name inCpp] \$prefix yes $args"]
  # All we need to do is register the type
  set knownTypes([$name inTcl]) $conv
  delete object $name
}

proc toplevelConst {name type {typemods {}}} {
  global declarations
  lappend declarations [new Variable $name $type $typemods no {const}]
}

proc toplevelVar {name type {typemods {}}} {
  global declarations
  lappend declarations [new Variable $name $type $typemods no {}]
}

proc toplevelFun {rtype name args} {
  global declarations
  lappend declarations [new Function $name $rtype $args no {}]
}

proc toplevelClass {ct name supers {body {}}} {
  global declarations
  lappend declarations [new Class $ct $name $supers $body]
}

proc unknowable name {
  global declarations
  lappend declarations [new Unknowable $name]
}

proc unsafe code {
  global declarations
  lappend declarations [new ExcludeFromSafe $code]
}

proc cxx args {
  global cxxoutfiles currentImplFile outfilesUntouched
  set cxxoutfiles [lassign $cxxoutfiles currentImplFile]
  lappend cxxoutfiles $currentImplFile
  foreach header $args {
    puts $currentImplFile "#include \"$header\""
  }
  set outfilesUntouched [lrange $outfilesUntouched 1 end]
}

proc predecclass args {
  foreach arg $args {
    puts $::cppout "class $arg;"
  }
}

proc const {args} {eval "toplevelConst $args"}
proc var {args} {eval "toplevelVar $args"}
proc fun {args} {eval "toplevelFun $args"}
proc class {args} {eval "toplevelClass $args"}
proc empty {} {}
proc template {name argspec objspec} {
  global templates
  set templates($name) yes

  proc instantiateTemplate_$name $argspec "
    set code {$objspec}
    foreach arg {$argspec} {
      set code \[regsub -all -- \"\\\\(\$arg\\\\)\" \$code \[set \$arg\]\]
    }
    eval \$code"
}

proc newFunType {ret args} {
  set argms {}
  set argts {}
  set argvts $args
  foreach arg $args {
    lappend argts [resolveType [lindex $arg 0]]
    if {[llength $arg]} {
      lappend argms [lrange $arg 1 [llength $arg]]
    } else {
      lappend argms ""
    }
  }
  if {$ret != "void"} {
    set rett [resolveType [lindex $ret 0]]
    set rettn [$rett getC++Name]
    set retm [lrange $ret 1 [llength $ret]]
    $rett setModifiers $retm
  } else {
    set rettn void
  }
  set cppType "DynFun[llength $argts]<$rettn"
  for {set i 0} {$i < [llength $argts]} {incr i} {
    set at [lindex $argts $i]
    $at setModifiers [lindex $argms $i]
    append cppType ",[$at getC++Name]"
  }
  append cppType ">"
  set tclType "fun<$rettn:[join $args ,]>"
  unknowable "${cppType}::fun_t ${tclType}::fun_t"
  class abstract-extendable "$cppType {$tclType}" {} "
    constructor default
    fun {$rettn} invoke purevirtual $args
    fun {$rettn} call static ${tclType}::fun_t* $args
    fun ${tclType}::fun_t* get"
}

set cxxtopmatter {
  /**
   * @file
   * @author C++-Tcl Bridge Code Generator
   * @brief Autogenerated bindings; <b>not intended for human consumption</b>
   *
   * AUTOGENERATED BY generate.tcl. DO NOT EDIT DIRECTLY.
   * @see src/tcl_iface/readme.txt
   */

  #include <map>
  #include <set>
  #include <vector>
  #include <string>
  #include <cstring>
  #include <cstdio>
  #include <cstdlib>
  #include <iostream>

  #include <GL/gl.h>
  #include <SDL.h>
  #include <tcl.h>
  #include <itcl.h>
  #include <libconfig.h++>

  #include "src/tcl_iface/bridge.hxx"
  #include "src/tcl_iface/implementation.hxx"
  #include "src/tcl_iface/dynfun.hxx"
  #include "src/exit_conditions.hxx"
  #include "src/globals.hxx"

  #pragma GCC diagnostic ignored "-Wunused-label"
  #pragma GCC diagnostic ignored "-Waddress"
  using namespace std;
  using namespace tcl_glue_implementation;
  using namespace libconfig;

  //Commands get their zeroth argument as their own name;
  //code generation is simpler if we drop this
  #define SHIFT ++objv, --objc
}

# Open our output files
file mkdir "tcl_iface/xxx"
set cppout [open "tcl_iface/bridge.cxx" w]
set tclout [open "../tcl/bridge.tcl" w]
set cxxoutfiles {}
set outfilesUntouched {}
for {set i 0} {$i < 16} {incr i} {
  set filename [format "tcl_iface/xxx/x%01x.cxx" $i]
  set f [open $filename w]
  puts $f $cxxtopmatter
  lappend cxxoutfiles $f
  lappend outfilesUntouched $filename
}

# Write topmatter for bridge.cxx
puts $cppout $cxxtopmatter

# Write topmatter for bridge.tcl
puts $tclout {
  package require Itcl

  # AUTOGENERATED BY generate.tcl. DO NOT EDIT DIRECTLY.
  # See readme.txt.

  #Our base Tcl glue code (eval'd to hardwire the magic cookie)
  eval [format {
    itcl::class AObject {
      # Ensure all code uses the fully-qualified name for C++ to recognize it
      method fqn {} {::return $this}

      protected constructor {cppClass constname cppArgs} {
        if {[::string length $constname]} {
          # Intercept errors so we don't leak the magic number
          if {[catch {
            "::c++ new $cppClass $constname" $this %s {*}$cppArgs
          } err]} {
            error "Bad constructor call: $cppClass $constname: $err"
          }
        }
      }
      destructor {
        {::c++ delete} $this %s
      }

      method super {sup cname {args {}}} {
        ${sup}::constructor $sup $cname $args
      }

      # Prevent access to the magic cookie by redirecting
      # all info requests to the global one
      method info {args} {
        ::namespace eval :: ::info {*}$args
      }
    }
  } ${ABENDSTERN-MAGIC-COOKIE-1} ${ABENDSTERN-MAGIC-COOKIE-1}]
  unset ABENDSTERN-MAGIC-COOKIE-1

  # Have rename commit suicide
  # rename rename {}

  set glueClass(AObject) yes

  proc new {clazz args} {
    global glueClass
    if {[info exists glueClass($clazz)]} {
      [$clazz #auto $clazz [lindex $args 0] [lrange $args 1 [llength $args]]] fqn
    } else {
      [$clazz #auto {*}$args] fqn
    }
  }
}

# Process definitions
source "tcl_iface/definition.tcl"

#Write top declarations
for {set i 0} {$i < [llength $declarations]} {incr i} {
  set dec [lindex $declarations $i]
  puts [$dec cget -implfile] [$dec generateC++Glue]
  puts $cppout [$dec generateC++PreDec]
  puts $tclout [$dec generateTclGlue]
}

#Write the init code
puts $cppout [format {
  Tcl_Interp* newInterpreter(bool safe, Tcl_Interp* master) %s
    Tcl_Interp* interp = newInterpreterImpl(safe,master);
    //Autodecs follow
} "{" ]
for {set i 0} {$i < [llength $declarations]} {incr i} {
  puts $cppout [[lindex $declarations $i] generateC++Dec]
  puts $tclout [[lindex $declarations $i] generateTclDec]
}
puts $cppout "newInterpreterImplPost(interp);"
puts $cppout "invokingInterpreter=master;"
puts $cppout "return interp; }"

puts $tclout "safe_source tcl/autosource.tcl"

close $cppout
close $tclout
foreach f $cxxoutfiles { close $f }

# If any of the outfiles haven't been touched, replace them with empty files.
foreach f $outfilesUntouched {
  close [open $f w]
}
