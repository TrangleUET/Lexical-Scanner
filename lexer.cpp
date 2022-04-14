#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <regex>

using namespace std;


const string SRC = "in.vc";
const string DST = "out.vctok";
const string DAT = "transition.dat";


set<string> keywords;
string endstates[25];

map<string, int> table[25];

string source_code;

vector<string> tokens;

void load_data();

void remove_comments( istream& file );

string reduce( char c );

void write_output( string des = "out.vctok" );

void inform_error_at( int line, int state, vector<char>& buffer );


int main( int argc, char const *argv[] )
{
	ifstream file;
	switch ( argc )
	{
		case 1:
			file.open( SRC );
			if ( !file.good() )
			{
				cerr << "Put VC source code in \"" << SRC << "\"";
				return -1;
			}
			break;
		case 2:
			file.open( argv[ 1 ] );
			if ( !file.good() )
			{
				cerr << "Can not find \"" << argv[1] << "\"";
				return -1;
			}
			break;
	}
	
	load_data();	
	remove_comments( file );

	int linecount = 1;

	int j = 0;
	char c;

	int state = 0;

	string input;

	bool error = false;

	vector<char> buffer;

	while ( c = source_code[ j++ ] )
	{
		if ( int( c ) < 0 ) break; 
		if ( c == '\n' ) ++linecount;

		input = reduce( c );
		if ( table[ state ].count( input ) == 0 )
		{
			if ( table[ state ].count( "other" ) == 0)
			{
				error = true;
				if ( state == 4 )
				{
					buffer.push_back( source_code[ j-1 ] );

					while ( isalpha( source_code[ j ] ) || isdigit( source_code[ j ] ) )
					{
						buffer.push_back( source_code[ j++ ] );
					}
				}
				
				inform_error_at(linecount, state, buffer);

				buffer.clear();
				state = 0;
				continue;
			}
			else 
			{
				if ( state == 1 && ( input == "letter" || input == "E" ) )
				{
					error = true;
					while ( isalpha( source_code[ j ] ) || isdigit( source_code[ j ] ) )
					{
						buffer.push_back( source_code[ j++ ] );
					}
					inform_error_at(linecount, state, buffer);
					buffer.clear();
					state = 0;
					continue;
				}
				else if ( state == 11 )
				{
					buffer.push_back(c);
					continue;
				}
				else --j;

				state = table[ state ][ "other" ];
			}
		}
		else 
		{
			if ( input != "ws" && input != "\"") buffer.push_back( c );
			state = table[ state ][ input ];
		}

		string word;
		if ( endstates[ state ] != "" )
		{
			word = { buffer.begin(), buffer.end() };
			buffer.clear();
			tokens.push_back( word );
			state = 0;
		}
	}

	if ( !error )
	{
		write_output();
		cout << "Sucessfully analyzed! Output written to \"" << DST << "\"" << endl;
	}
	return 0;
}


void load_data()
{
	ifstream file;
	file.open( DAT );
	if ( !file.good() )
	{
		cerr << "Khong tim thay \"" << DAT << "\"";
		exit( -1 );
	}
	string line;
	string word;
	getline( file, line ); 

	getline( file, line ); 
	stringstream ss( line );
	while ( ss >> word )
		keywords.insert( word );
	ss.clear();

	getline( file, line ); 

	getline( file, line );
	int state;
	string attr;
	while ( getline( file, line ) && line != "" )
	{
		ss.str( line );
		ss >> state >> attr;
		endstates[ state ] = attr;
		ss.clear();
	}

	getline( file, line ); 

	regex pt( "[\\w\\.\"<>!=|&+*/-]+ +\\d+" ); 
	smatch sm;

	string input;
	int output;
	while ( getline( file, line ) )
	{
		state = stoi( line.substr( 0, 2 ) );
		while ( regex_search( line, sm, pt ) ) 
		{
			ss.str( sm[ 0 ] );
			ss >> input >> output;
			table[ state ][ input ] = output;
			line = sm.suffix().str();
			ss.clear();
		}
	}
}

void remove_comments( istream& file )
{
	source_code = { istreambuf_iterator<char>(file), istreambuf_iterator<char>{} };
	int n = source_code.length();

	vector<char> res(n);
	for ( size_t i = 0; i < n; i++ )
		res[i] = ' ';	

	bool single_cmt = false;
	bool multi_cmt = false;

	for ( int i = 0; i < n; ++i )
	{		
		if ( single_cmt == true && source_code[i] == '\n' )
		{
			single_cmt = false;
			res[ i ] = '\n';
		}

		else if ( multi_cmt == true && source_code[ i ] == '*' && source_code[ i+1 ] == '/' )
			multi_cmt = false, i++;

		else if ( single_cmt || multi_cmt )
		{
			if ( source_code[ i ] == '\n' )
				res[ i ] = source_code[ i ];
		}
		else if ( source_code[ i ] == '/' && source_code[ i+1 ] == '/' )
			single_cmt = true, i++;
		else if ( source_code[ i ] == '/' && source_code[ i+1 ] == '*' )
			multi_cmt = true, i++;
		else res[ i ] = source_code[ i ];
	}
	for(size_t i=0;i<n;i++)
		source_code[i]=res[i];
}

string reduce( char c )
{
	if ( c == 'e' || c == 'E' ) return "E"; 
	if ( 'a' <= c && c <= 'z' || 'A' <= c && c <= 'Z' || c == '_' ) return "letter";
	if ( '0' <= c && c <= '9' ) return "digit";
	if ( c == '{' || c == '}'|| c == '['|| c == ']' || c == '('|| c == ')'|| c == ';'|| c == ',' )
		return "ws";
	if ( c == ' ' || c == '\t' || c == '\n' ) return "ws";
	return string( 1, c );
}

void write_output( string des )
{
	ofstream file( des );
	for ( const string& s : tokens )
		file << s << '\n';
}

void inform_error_at( int line, int state, vector<char>& buffer )
{
	cout << SRC << ": line " << line << ": \"" 
	     << string( buffer.begin(), buffer.end() ) << "\"" 
		  << " is not a valid " 
		  << ( state != 21 && state != 23 ? "indentifier" : "operator" ) << endl;
}