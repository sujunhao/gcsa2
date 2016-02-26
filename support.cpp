/*
  Copyright (c) 2015, 2016 Genome Research Ltd.

  Author: Jouni Siren <jouni.siren@iki.fi>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include "support.h"

namespace gcsa
{

//------------------------------------------------------------------------------

/*
  The default alphabet interprets \0 and $ as endmarkers, ACGT and acgt as ACGT,
  # as a the label of the source node, and the and the remaining characters as N.
*/

const sdsl::int_vector<8> Alphabet::DEFAULT_CHAR2COMP =
{
  0, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 5, 5, 6,  0, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,

  5, 1, 5, 2,  5, 5, 5, 3,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 5, 5, 5,  4, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 1, 5, 2,  5, 5, 5, 3,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 5, 5, 5,  4, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,

  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,

  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5
};

const sdsl::int_vector<8> Alphabet::DEFAULT_COMP2CHAR = { '$', 'A', 'C', 'G', 'T', 'N', '#' };

//------------------------------------------------------------------------------

Alphabet::Alphabet() :
  char2comp(DEFAULT_CHAR2COMP), comp2char(DEFAULT_COMP2CHAR),
  C(DEFAULT_COMP2CHAR.size() + 1, 0),
  sigma(DEFAULT_COMP2CHAR.size())
{
}

Alphabet::Alphabet(const Alphabet& a)
{
  this->copy(a);
}

Alphabet::Alphabet(Alphabet&& a)
{
  *this = std::move(a);
}

Alphabet::Alphabet(const sdsl::int_vector<64>& counts,
  const sdsl::int_vector<8>& _char2comp, const sdsl::int_vector<8>& _comp2char) :
  char2comp(_char2comp), comp2char(_comp2char),
  C(_comp2char.size() + 1, 0),
  sigma(_comp2char.size())
{
  for(size_type i = 0; i < counts.size(); i++) { this->C[i + 1] = this->C[i] + counts[i]; }
}

Alphabet::~Alphabet()
{
}

void
Alphabet::copy(const Alphabet& a)
{
  this->char2comp = a.char2comp;
  this->comp2char = a.comp2char;
  this->C = a.C;
  this->sigma = a.sigma;
}

void
Alphabet::swap(Alphabet& a)
{
  if(this != &a)
  {
    this->char2comp.swap(a.char2comp);
    this->comp2char.swap(a.comp2char);
    this->C.swap(a.C);
    std::swap(this->sigma, a.sigma);
  }
}

Alphabet&
Alphabet::operator=(const Alphabet& a)
{
  if(this != &a) { this->copy(a); }
  return *this;
}

Alphabet&
Alphabet::operator=(Alphabet&& a)
{
  if(this != &a)
  {
    this->char2comp = std::move(a.char2comp);
    this->comp2char = std::move(a.comp2char);
    this->C = std::move(a.C);
    this->sigma = a.sigma;
  }
  return *this;
}

Alphabet::size_type
Alphabet::serialize(std::ostream& out, sdsl::structure_tree_node* v, std::string name) const
{
  sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
  size_type written_bytes = 0;
  written_bytes += this->char2comp.serialize(out, child, "char2comp");
  written_bytes += this->comp2char.serialize(out, child, "comp2char");
  written_bytes += this->C.serialize(out, child, "C");
  written_bytes += sdsl::write_member(this->sigma, out, child, "sigma");
  sdsl::structure_tree::add_size(child, written_bytes);
  return written_bytes;
}

void
Alphabet::load(std::istream& in)
{
  this->char2comp.load(in);
  this->comp2char.load(in);
  this->C.load(in);
  sdsl::read_member(this->sigma, in);
}

//------------------------------------------------------------------------------

SadaSparse::SadaSparse()
{
}

SadaSparse::SadaSparse(const SadaSparse& source)
{
  this->copy(source);
}

SadaSparse::SadaSparse(SadaSparse&& source)
{
  *this = std::move(source);
}

SadaSparse::~SadaSparse()
{
}

void
SadaSparse::swap(SadaSparse& another)
{
  if(this != &another)
  {
    this->ones.swap(another.ones);
    sdsl::util::swap_support(this->one_rank, another.one_rank,
      &(this->ones), &(another.ones));

    this->filter.swap(another.filter);
    sdsl::util::swap_support(this->filter_rank, another.filter_rank,
      &(this->filter), &(another.filter));

    this->values.swap(another.values);
    sdsl::util::swap_support(this->value_select, another.value_select,
      &(this->values), &(another.values));
  }
}

SadaSparse&
SadaSparse::operator=(const SadaSparse& source)
{
  if(this != &source) { this->copy(source); }
  return *this;
}

SadaSparse&
SadaSparse::operator=(SadaSparse&& source)
{
  if(this != &source)
  {
    this->ones = std::move(source.ones);
    this->one_rank = std::move(source.one_rank);

    this->filter = std::move(source.filter);
    this->filter_rank = std::move(source.filter_rank);

    this->values = std::move(source.values);
    this->value_select = std::move(source.value_select);

    this->setVectors();
  }
  return *this;
}

SadaSparse::size_type
SadaSparse::serialize(std::ostream& out, sdsl::structure_tree_node* v, std::string name) const
{
  sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
  size_type written_bytes = 0;

  written_bytes += this->ones.serialize(out, child, "ones");
  written_bytes += this->one_rank.serialize(out, child, "one_rank");

  written_bytes += this->filter.serialize(out, child, "filter");
  written_bytes += this->filter_rank.serialize(out, child, "filter_rank");

  written_bytes += this->values.serialize(out, child, "values");
  written_bytes += this->value_select.serialize(out, child, "value_select");

  sdsl::structure_tree::add_size(child, written_bytes);
  return written_bytes;
}

void
SadaSparse::load(std::istream& in)
{
  this->ones.load(in);
  this->one_rank.load(in, &(this->ones));

  this->filter.load(in);
  this->filter_rank.load(in, &(this->filter));

  this->values.load(in);
  this->value_select.load(in, &(this->values));
}

void
SadaSparse::copy(const SadaSparse& source)
{
  this->ones = source.ones;
  this->one_rank = source.one_rank;

  this->filter = source.filter;
  this->filter_rank = source.filter_rank;

  this->values = source.values;
  this->value_select = source.value_select;

  this->setVectors();
}

void
SadaSparse::setVectors()
{
  this->one_rank.set_vector(&(this->ones));
  this->filter_rank.set_vector(&(this->filter));
  this->value_select.set_vector(&(this->values));
}

//------------------------------------------------------------------------------

SadaRLE::SadaRLE()
{
}

SadaRLE::SadaRLE(const SadaRLE& source)
{
  this->copy(source);
}

SadaRLE::SadaRLE(SadaRLE&& source)
{
  *this = std::move(source);
}

SadaRLE::~SadaRLE()
{
}

void
SadaRLE::swap(SadaRLE& another)
{
  if(this != &another)
  {
    this->ones.swap(another.ones);
    sdsl::util::swap_support(this->one_rank, another.one_rank,
      &(this->ones), &(another.ones));

    this->heads.swap(another.heads);
    sdsl::util::swap_support(this->head_select, another.head_select,
      &(this->heads), &(another.heads));

    this->lengths.swap(another.lengths);
    sdsl::util::swap_support(this->length_rank, another.length_rank,
      &(this->lengths), &(another.lengths));
    sdsl::util::swap_support(this->length_select, another.length_select,
      &(this->lengths), &(another.lengths));
  }
}

SadaRLE&
SadaRLE::operator=(const SadaRLE& source)
{
  if(this != &source) { this->copy(source); }
  return *this;
}

SadaRLE&
SadaRLE::operator=(SadaRLE&& source)
{
  if(this != &source)
  {
    this->ones = std::move(source.ones);
    this->one_rank = std::move(source.one_rank);

    this->heads = std::move(source.heads);
    this->head_select = std::move(source.head_select);

    this->lengths = std::move(source.lengths);
    this->length_rank = std::move(source.length_rank);
    this->length_select = std::move(source.length_select);

    this->setVectors();
  }
  return *this;
}

SadaRLE::size_type
SadaRLE::serialize(std::ostream& out, sdsl::structure_tree_node* v, std::string name) const
{
  sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
  size_type written_bytes = 0;

  written_bytes += this->ones.serialize(out, child, "ones");
  written_bytes += this->one_rank.serialize(out, child, "one_rank");

  written_bytes += this->heads.serialize(out, child, "heads");
  written_bytes += this->head_select.serialize(out, child, "head_select");

  written_bytes += this->lengths.serialize(out, child, "lengths");
  written_bytes += this->length_rank.serialize(out, child, "length_rank");
  written_bytes += this->length_select.serialize(out, child, "length_select");

  sdsl::structure_tree::add_size(child, written_bytes);
  return written_bytes;
}

void
SadaRLE::load(std::istream& in)
{
  this->ones.load(in);
  this->one_rank.load(in, &(this->ones));

  this->heads.load(in);
  this->head_select.load(in, &(this->heads));

  this->lengths.load(in);
  this->length_rank.load(in, &(this->lengths));
  this->length_select.load(in, &(this->lengths));
}

void
SadaRLE::copy(const SadaRLE& source)
{
  this->ones = source.ones;
  this->one_rank = source.one_rank;

  this->heads = source.heads;
  this->head_select = source.head_select;

  this->lengths = source.lengths;
  this->length_rank = source.length_rank;
  this->length_select = source.length_select;

  this->setVectors();
}

void
SadaRLE::setVectors()
{
  this->one_rank.set_vector(&(this->ones));
  this->head_select.set_vector(&(this->heads));
  this->length_rank.set_vector(&(this->lengths));
  this->length_select.set_vector(&(this->lengths));
}

//------------------------------------------------------------------------------

std::string
Key::decode(key_type key, size_type kmer_length, const Alphabet& alpha)
{
  key = label(key);
  kmer_length = std::min(kmer_length, MAX_LENGTH);

  std::string res(kmer_length, '\0');
  for(size_type i = 1; i <= kmer_length; i++)
  {
    res[kmer_length - i] = alpha.comp2char[key & CHAR_MASK];
    key >>= CHAR_WIDTH;
  }

  return res;
}

void
Key::lastChars(const std::vector<key_type>& keys, sdsl::int_vector<0>& last_char)
{
  sdsl::util::clear(last_char);
  last_char = sdsl::int_vector<0>(keys.size(), 0, CHAR_WIDTH);
  for(size_type i = 0; i < keys.size(); i++) { last_char[i] = Key::last(keys[i]); }
}

//------------------------------------------------------------------------------

node_type
Node::encode(const std::string& token)
{
  size_t separator = 0;
  size_type node = std::stoul(token, &separator);
  if(separator + 1 >= token.length())
  {
    std::cerr << "Node::encode(): Invalid position token " << token << std::endl;
    return 0;
  }

  bool reverse_complement = false;
  if(token[separator + 1] == '-')
  {
    reverse_complement = true;
    separator++;
  }

  std::string temp = token.substr(separator + 1);
  size_type offset = std::stoul(temp);
  if(offset > OFFSET_MASK)
  {
    std::cerr << "Node::encode(): Offset " << offset << " too large" << std::endl;
    return 0;
  }

  return encode(node, offset, reverse_complement);
}

std::string
Node::decode(node_type node)
{
  std::ostringstream ss;
  ss << id(node) << ':';
  if(rc(node)) { ss << '-'; }
  ss << offset(node);
  return ss.str();
}

//------------------------------------------------------------------------------

KMer::KMer()
{
}

KMer::KMer(const std::vector<std::string>& tokens, const Alphabet& alpha, size_type successor)
{
  byte_type predecessors = chars(tokens[2], alpha);
  byte_type successors = chars(tokens[3], alpha);
  this->key = Key::encode(alpha, tokens[0], predecessors, successors);
  this->from = Node::encode(tokens[1]);
  this->to = Node::encode(tokens[successor]);
}

byte_type
KMer::chars(const std::string& token, const Alphabet& alpha)
{
  byte_type val = 0;
  for(size_type i = 0; i < token.length(); i += 2) { val |= 1 << alpha.char2comp[token[i]]; }
  return val;
}

std::ostream&
operator<< (std::ostream& out, const KMer& kmer)
{
  out << "(key " << Key::label(kmer.key)
      << ", in " << (size_type)(Key::predecessors(kmer.key))
      << ", out " << (size_type)(Key::successors(kmer.key))
      << ", from " << Node::decode(kmer.from)
      << ", to " << Node::decode(kmer.to) << ")";
  return out;
}

//------------------------------------------------------------------------------

} // namespace gcsa
