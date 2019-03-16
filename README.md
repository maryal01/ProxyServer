# NetworksFinalProject

##Modules

## Module 1 Parsing the request and getting the data
	* support both HTTP and HTTPS protocol
	* handle concurrent connection
	* parsing the incoming request into the representation of client request in the program

##Module 2 Sending the response
	* checking if the data is present in proxy
		* if not, request the data and store it in proxy
		* if present, get the data and return to Module 2  	
	* maintain the headers to respond
	* return the response back with the updated header

## Module 3 Caching
	* optimize the caching far faster data fetching -- hashtable
	* create following endpoints of caching
		* get the data based given url
		* store the data
		* remove the data given url
		* remove all elements of cache
		* print all elements of cache
		* return least accessed
	
	
## Module 5 Build Transcoding Support in your proxy

