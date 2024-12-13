# Formal definition of an RDF file

- an RDF file also knonwn as Redirectoin file is a single layer in an composable layer stack that will produce an redirection Dict.
- the individual layers are not availalbe for change only the top loaded layer has options to change sublayers and redirectoin data
- redirectoin data is highest oder. aka: the first file loaded hast the highest strenght and will overwrite lower layer entrys.

## Constrcution of an RDF

an rdf is stored as json and generated via the redirectoinFile class.

```json
{
  "subLayers": [],
  "data": {}
}
```

sublayers are lower layers loaded secuentially and strenght ordert the first one in the list will be the strongest.
the data regestry represents the redirection keys inserted by this file. they will allways be stronger than the sublayers.

it is required that every RDF will have those 2 data stores.
