# Downloads and (re)mounts all genetic AI data.

if {![info exists aidownld_firstRun]} {
  set aidownld_firstRun yes
}
# On the first run, handle all cached data
if {$aidownld_firstRun
&&  [$ exists conf.game.use_geneticai]
&&  [$ bool conf.game.use_geneticai]} {
  file mkdir dna
  if {[catch {
    $ open dna/ai_list.dat genai
  } err]} {
    log "Could not mount genai: $err"
    $ create dna/ai_list.dat genai
    $ add genai species STGroup
  }

  for {set i 0} {$i < [$ length genai.species]} {incr i} {
    set name [$ name genai.species.\[$i\]]
    if {[catch {
      $ open dna/$name.dna ai:$name
    } err]} {
      log "Could not mount ai:$name: $err"
      $ remix genai.species $i
      incr i -1
    }
  }
}

proc aidownld_downloadingList {} {
  if {$::abnet::busy} { return 0 }
  if {$::abnet::success} {
    $ unmodify genai
    $ close genai
    if {[catch {
      $ open dna/ai_list.dat genai
    } err]} {
      log "Could not mount downloaded genai: $err"
      # Create empty
      $ create dna/ai_list.dat genai
      $ add genai species STGroup
    }
    $::state setCallback [_ A boot aidownld] aidownld_downloadingDNA
    set ::aidownld_index -1
    set ::aidownld_needRemount no
    return 0 ;# Don't cut the next callback off
  }
  log "Download of ai_list.dat failed: $::abnet::resultMessage"
  # Failure
  return 200
}

proc aidownld_downloadingDNA {} {
  global aidownld_index aidownld_needRemount

  if {$::abnet::busy} {
    return [expr {$aidownld_index*100 / [$ length genai.species]}]
  }
  if {!$::abnet::success} {
    log "Download of AI data failed: $::abnet::resultMessage"
    return 200
  }

  # If downloaded, remount
  if {$aidownld_needRemount} {
    set name [$ name genai.species.\[$aidownld_index\]]
    $ close ai:$name
    if {[catch {
      $ open dna/$name.dna ai:$name
    } err]} {
      log "Could not mount downloaded ai:$name: $err"
      $ remix genai.species $aidownld_index
      incr aidownld_index -1
    }
  }

  # Get next file if needed
  incr aidownld_index
  if {$aidownld_index >= [$ length genai.species]} {
    return 200 ;# Nothing more
  }
  # Check generations
  set name [$ name genai.species.\[$aidownld_index\]]
  set curr -1
  catch {
    set curr [$ int ai:$name.generation]
  }
  if {$curr != [$ int genai.species.\[$aidownld_index\]]} {
    # Differing generations (or we just don't have it)
    ::abnet::getfn $name.dna dna/$name.dna 0
    set aidownld_needRemount yes
  } else {
    # Cached is correct
    set aidownld_needRemount no
  }

  return [expr {$aidownld_index*100 / [$ length genai.species]}]
}

if {$::abnet::isConnected
&&  [$ exists conf.game.use_geneticai]
&&  [$ bool conf.game.use_geneticai]} {
  ::abnet::getfn CAI dna/ai_list.dat 0
  $state setCallback [_ A boot aidownld] aidownld_downloadingList
}
