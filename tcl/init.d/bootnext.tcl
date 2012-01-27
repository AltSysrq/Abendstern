# Move to the bootgraph or headless boot catalogues
if {!$headless} {
  $state setCatalogue bootgraph
} else {
  $state setCatalogue headless
}
