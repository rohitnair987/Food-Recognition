#include <iostream>
#include <fstream>
using namespace std;

class EigenFood : public Classifier
{
public:
  EigenFood(const vector<string> &_class_list) : Classifier(_class_list) {}
  int dir_count=1;
  int count = 1;

  // EigenFood training. All this does is read in all the images, resize
  // them to a common size, convert to greyscale, and dump them as vectors to a file
  virtual void train(const Dataset &filenames)
  {
    for(Dataset::const_iterator c_iter=filenames.begin(); c_iter != filenames.end(); ++c_iter)
      {
	cout << endl << "Processing " << c_iter->first << endl;
	int cols = size*size*3;
	int rows = filenames.size();

	CImg<double> class_vectors(cols, rows, 1);

	// convert each image to be a row of this "model" image
	for(int i=0; i<c_iter->second.size(); i++)
	  class_vectors = class_vectors.draw_image(0, i, 0, 0, extract_features(c_iter->second[i].c_str()));

	//calculating average food vector
	CImg<double> avg_vector(cols,1,1);
	double sum;
	for(int c=0; c<cols; c++) {
		sum = 0.0;
		for(int r=0; r<rows; r++) {
			sum += class_vectors(c,r,0,0);
		}
		avg_vector(c,0,0,0) = sum/rows;
	}

	//calculating normalized image vectors
	CImg<double> class_vectors_normalized = class_vectors;
	for(int c=0; c<cols; c++)
		for(int r=0; r<rows; r++)
			class_vectors_normalized(c,r,0,0) = class_vectors(c,r,0,0) - avg_vector(c,0,0,0);

	//covariance
	CImg<double> covariance = class_vectors_normalized * transpose(class_vectors_normalized);// / rows;

	CImg<> U,S,V;
	covariance.symmetric_eigen(S, U);
//	covariance.SVD(U,S,V);

	//select 10 best eigenvectors
	CImg<> eigenvectors(10,25,1);
	int select_rows = 10;
	for(int i = 0; i < select_rows; i++)
		for(int j = 0; j < rows; j++)
			eigenvectors(i,j,0,0) = U(i,j,0,0);

	CImg<> class_vectors_from_eigen = transpose(eigenvectors) * class_vectors;
//							10x1200	  =	  25x10    *    25x1200

	CImg<double> printeigenvectors(30,30,1,1);
	string filename;
	int dims = 30;
	for(int k = 0; k < select_rows; k++) {
		for(int i = 0; i < dims; i++)
			for(int j = 0; j < dims; j++)
				printeigenvectors(i,j,0,0) = class_vectors_from_eigen(k,i+j,0,0);
//		printeigenvectors.save_png(("printeigenvectors_" + k + ".png").c_str());
	}

	//Checking correctness ... whether U * U.T = I
//	CImg<> identity = U * transpose(U);
//	cout << "U.UT:\n";
//	for(int r=0; r<25; r++){
//		cout << r << ": ";
//		for(int c=0; c<25; c++)
//			cout << (int)identity(r,c,0,0) << " ";
//		cout << endl;
//	}

	//testing --> printing a single eigenvector as an image
//	CImg<> test = avg_vector;
//	for(int i = 0; i < cols; i++)
//		test(i,0,0,0) += class_vectors_from_eigen(i,0,0,0);
//	test.save_png("test.png".c_str());

	class_vectors_from_eigen.save_png(("eigenfood_model_" + c_iter->first + ".png").c_str());
      }
  }

  virtual string classify(const string &filename)
  {
	  //svm
	  	CImg<double> test_image = extract_features(filename);
	      pair<string, double> best("", 10e100);
	      double this_cost;
	      for(int c=0; c<class_list.size(); c++)
	        for(int row=0; row<models[ class_list[c] ].height(); row++)
	  	if((this_cost = (test_image - models[ class_list[c] ].get_row(row)).magnitude()) < best.second)
	  	  best = make_pair(class_list[c], this_cost);
	      return best.first;

	      //nearest neighbor
	      /*ofstream myfile;
	  	myfile.open("testing.txt");
	  	CImg<double> test_image = extract_features(filename);
	  	if (count == 11){
	  		dir_count++;
	  		count = 1;
	  		myfile << dir_count<<" ";
	  	}
	  	else{
	  		myfile << dir_count<<" ";
	  		count++;
	  	}
	  	for( int j = 0; j < test_image.size(); j++)
	  		myfile << j+1 << ":"<< test_image[j] << " " ;
	  	myfile <<'\n';
	  	myfile.close();
	  	system("./svm_multiclass_classify testing.txt svm_struct_model");
	  	fstream myfile_input("svm_predictions", std::ios_base::in);
	  	int x;
	  	myfile_input>>x;
	  	myfile_input.close();
	  	return class_list[x-1];*/
  }

  virtual void load_model()
  {
    for(int c=0; c < class_list.size(); c++)
      models[class_list[c] ] = (CImg<double>(("eigenfood_model_" + class_list[c] + ".png").c_str()));
  }
protected:
  // extract features from an image, which in this case just involves resampling and
  // rearranging into a vector of pixel data.
  CImg<double> extract_features(const string &filename)
    {
      return (CImg<double>(filename.c_str())).resize(size,size,1,3).unroll('x');
    }

  static const int size=20;  // subsampled image resolution
  map<string, CImg<double> > models; // trained models
};
