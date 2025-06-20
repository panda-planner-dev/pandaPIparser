#ifndef __ENGLISHTRANSCRIPTIONWRITER_HPP
#define __ENGLISHTRANSCRIPTIONWRITER_HPP

#include <iostream> // For ostream

// Forward declarations if needed later (e.g., for types from parsetree.hpp)
// struct parsed_task;
// struct parsed_method;

void write_instance_as_english_transcription(std::ostream& domain_out, std::ostream& problem_out);

#endif // __ENGLISHTRANSCRIPTIONWRITER_HPP
