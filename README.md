# NetworksFinalProject

# Functionality
## Module 1 Parsing the request and getting the data (Bakar Tavadze) 
	- support basic GET method for HTTP and HTTPS protocol
	- handle concurrent connection
	- parse the incoming request into the representation of client request in the program

## Module 2 Sending the response (Bakar Tavadze)
	- checking if the data is present in proxy
		* if not, request the data and store it in proxy
		* if present, get the data and return to Module 2  	
	- maintain the headers to respond
	- return the response back with the updated header

## Module 3 Optimized Caching (Manish Aryal) 
	- optimize the caching far faster data fetching -- hashtable
	- create following endpoints of caching
		* get the data based given url
		* store the data
		* remove the data given url
		* remove all elements of cache
		* print all elements of cache
		* return least accessed
	
## Module 4 Build Transcoding Support in your proxy (Sang Woo Kim)
	- build following transcoding features
		* option to recieve only plain html text without image
		* if requested link is an image, option to get lower resolution image

## Module 5 Limiting Bandwidth (Sang Woo Kim)
	- limit bandwith for following reasons
		* requests huge amount of data within certain period of time 
		* requests data frequently

## Module 6 Advanced Caching Policy (Manish Aryal)
	- if a website has a link, cache the links
	- cache the static images embedded in the website
		* return the images based on user option of getting plain text or text with image

## Modules for Testing (Sang Woo Kim, Manish Aryal, Bakar Tavadze)
	- test all the basic functionality
	- test all additional features work 
