#include "monitor.h"
// ======= HELPER FUNCTIONS ============

void unique(std::vector<std::string> &keywords)
{
  std::vector<std::string>::iterator it;
  std::sort(keywords.begin(), keywords.end());
  it = std::unique(keywords.begin(), keywords.end());
  keywords.resize(std::distance(keywords.begin(), it));
}

void split(std::string sentence, std::string delimiter, std::vector<std::string> &words)
{
  size_t dpos = 0;
  std::string tmp;
  sentence += delimiter;
  while ((dpos = sentence.find(delimiter)) != std::string::npos)
  {
    tmp = sentence.substr(0, dpos);
    words.push_back(tmp);
    sentence.erase(0, dpos + delimiter.length());
  }
}

std::string Monitor::getAllProducts(std::string domain)
{
  std::string url = "https://" + domain + "/products.json";
  auto r1 = cpr::Get(cpr::Url(url));
  if (r1.status_code >= 400)
  {
    std::cout << "Page returned a " << std::to_string(r1.status_code) << " error\n";
    return "Error";
  }
  else if (r1.status_code == 0)
  {
    return "Page not found";
  }
  else
  {
    return r1.text;
  }
};

std::string Monitor::getAllCleaned(std::string domain)
{
  ProductCollection pc = ProductCollection();
  int i = 0;
  std::string products = Monitor::getAllProducts(domain);

  if (products != "Page not found" && products != "Error")
  {
    rapidjson::Document doc;
    doc.Parse(products.c_str());
    assert(doc["products"].IsArray());
    const rapidjson::Value &p = doc["products"];
    for (rapidjson::SizeType i = 0; i < p.Size(); ++i)
    {
      std::string title = p[i]["title"].GetString();
      std::transform(title.begin(), title.end(), title.begin(), ::tolower);

      std::string id = std::to_string(p[i]["id"].GetInt64());
      std::string handle = p[i]["handle"].GetString();
      std::string image = p[i]["images"][0]["src"].GetString();
      pc.createProduct(id, title, handle, image);

      // Loop through our variants
      const rapidjson::Value &var = doc["products"][i]["variants"];
      for (rapidjson::SizeType i = 0; i < var.Size(); i++)
      {
        std::string id2 = std::to_string(var[i]["id"].GetInt64());
        std::string vtitle = var[i]["title"].GetString();
        std::string sku = var[i]["sku"].GetString();
        std::string price = var[i]["price"].GetString();
        std::string available;
        if (var[i]["available"].GetBool())
        {
          available = "true";
        }
        else
        {
          available = "false";
        }
        pc.addVariant(id, id2, vtitle, sku, price, available);
      }
    }
    std::string response = pc.getProducts();
    return response;
  }
  else
  {
    return products;
  }
}

std::string Monitor::findProductByTitle(std::string domain, std::vector<std::string> keywords)
{
  ProductCollection pc = ProductCollection();
  int i = 0;
  std::vector<std::string>::iterator itr;
  std::vector<std::string> foundtitle;
  unique(keywords);
  std::string products = Monitor::getAllProducts(domain);

  if (products != "Page not found" && products != "Error")
  {
    rapidjson::Document doc;
    doc.Parse(products.c_str());
    assert(doc["products"].IsArray());
    const rapidjson::Value &p = doc["products"];

    for (itr = keywords.begin(); itr != keywords.end(); ++itr)
    {
      for (rapidjson::SizeType i = 0; i < p.Size(); ++i)
      {
        std::string kw = *itr;
        std::string title = p[i]["title"].GetString();
        std::transform(kw.begin(), kw.end(), kw.begin(), ::tolower);
        std::transform(title.begin(), title.end(), title.begin(), ::tolower);

        if (title.find(kw) != std::string::npos)
        {
          //std::cout << "Found in title: " << title << std::endl;
          //std::cout << "Matched keyword: " << kw << std::endl;

          //BUILD OUR PRODUCT
          std::string id = std::to_string(p[i]["id"].GetInt64());
          std::string handle = p[i]["handle"].GetString();
          std::string image = p[i]["images"][0]["src"].GetString();
          pc.createProduct(id, title, handle, image);

          // Loop through our variants
          const rapidjson::Value &var = doc["products"][i]["variants"];
          for (rapidjson::SizeType i = 0; i < var.Size(); i++)
          {
            std::string id2 = std::to_string(var[i]["id"].GetInt64());
            std::string vtitle = var[i]["title"].GetString();
            std::string sku = var[i]["sku"].GetString();
            std::string price = var[i]["price"].GetString();
            std::string available;
            if (var[i]["available"].GetBool())
            {
              available = "true";
            }
            else
            {
              available = "false";
            }
            pc.addVariant(id, id2, vtitle, sku, price, available);
          }
          foundtitle.push_back(title);
        }
      }
    }
    //For each product in product title and product handle eliminate all that dont contain a kw
    unique(foundtitle);
    for (itr = keywords.begin(); itr != keywords.end(); ++itr)
    {
      std::string kw = *itr;
      std::vector<std::string>::iterator itr2;
      for (itr2 = foundtitle.begin(); itr2 != foundtitle.end(); ++itr2)
      {
        std::string title = *itr2;
        if (title.find(kw) == std::string::npos)
        {
          foundtitle.erase(itr2--);
        }
      }
    }
    std::string response = "";
    for (itr = foundtitle.begin(); itr != foundtitle.end(); ++itr)
    {
      Product *pt = pc.findProductByTitle(*itr);
      if (pt != nullptr)
      {
        response += pt->getProduct();
        response += ",";
      }
      delete pt;
    }
    if (response != "")
    {
      response.pop_back();
    }
    response = "[" + response + "]";

    return response;
  }
  else
  {
    return products;
  }
  //BUILD PRODUCT OBJECT OR RETURN ARRAY OF PRODUCTS MATCHING?
  // For each product in our array create a json object
}

std::string Monitor::searchProductByTitle(std::string product, std::vector<std::string> keywords)
{
  ProductCollection pc = ProductCollection();
  int i = 0;
  std::vector<std::string>::iterator itr;
  std::vector<std::string> foundtitle;
  unique(keywords);
  std::string products = product;

  if (products != "Page not found" && products != "Error")
  {
    rapidjson::Document doc;
    doc.Parse(products.c_str());
    assert(doc["products"].IsArray());
    const rapidjson::Value &p = doc["products"];

    for (itr = keywords.begin(); itr != keywords.end(); ++itr)
    {
      for (rapidjson::SizeType i = 0; i < p.Size(); ++i)
      {
        std::string kw = *itr;
        std::string title = p[i]["title"].GetString();
        std::transform(kw.begin(), kw.end(), kw.begin(), ::tolower);
        std::transform(title.begin(), title.end(), title.begin(), ::tolower);

        if (title.find(kw) != std::string::npos)
        {
          //std::cout << "Found in title: " << title << std::endl;
          //std::cout << "Matched keyword: " << kw << std::endl;

          //BUILD OUR PRODUCT
          std::string id = std::to_string(p[i]["id"].GetInt64());
          std::string handle = p[i]["handle"].GetString();
          std::string image = p[i]["images"][0]["src"].GetString();
          pc.createProduct(id, title, handle, image);

          // Loop through our variants
          const rapidjson::Value &var = doc["products"][i]["variants"];
          for (rapidjson::SizeType i = 0; i < var.Size(); i++)
          {
            std::string id2 = std::to_string(var[i]["id"].GetInt64());
            std::string vtitle = var[i]["title"].GetString();
            std::string sku = var[i]["sku"].GetString();
            std::string price = var[i]["price"].GetString();
            std::string available;
            if (var[i]["available"].GetBool())
            {
              available = "true";
            }
            else
            {
              available = "false";
            }
            pc.addVariant(id, id2, vtitle, sku, price, available);
          }
          foundtitle.push_back(title);
        }
      }
    }
    //For each product in product title and product handle eliminate all that dont contain a kw
    unique(foundtitle);
    for (itr = keywords.begin(); itr != keywords.end(); ++itr)
    {
      std::string kw = *itr;
      std::vector<std::string>::iterator itr2;
      for (itr2 = foundtitle.begin(); itr2 != foundtitle.end(); ++itr2)
      {
        std::string title = *itr2;
        if (title.find(kw) == std::string::npos)
        {
          foundtitle.erase(itr2--);
        }
      }
    }
    std::string response = "";
    for (itr = foundtitle.begin(); itr != foundtitle.end(); ++itr)
    {
      Product *pt = pc.findProductByTitle(*itr);
      if (pt != nullptr)
      {
        response += pt->getProduct();
        response += ",";
      }
      delete pt;
    }
    if (response != "")
    {
      response.pop_back();
    }
    response = "[" + response + "]";

    return response;
  }
  else
  {
    return products;
  }
  //BUILD PRODUCT OBJECT OR RETURN ARRAY OF PRODUCTS MATCHING?
  // For each product in our array create a json object
}

std::string Monitor::outofstockSizes(std::string domain, std::string productID)
{
  ProductCollection pc = ProductCollection();
  int i = 0;
  std::string products = Monitor::getAllProducts(domain);
  if (products != "Page not found" && products != "Error")
  {
    rapidjson::Document doc;
    doc.Parse(products.c_str());
    assert(doc["products"].IsArray());
    const rapidjson::Value &p = doc["products"];

    for (rapidjson::SizeType i = 0; i < p.Size(); ++i)
    {
      std::string id = std::to_string(p[i]["id"].GetInt64());
      std::string handle = p[i]["handle"].GetString();
      if (id == productID)
      {
        std::string title = p[i]["title"].GetString();
        std::transform(title.begin(), title.end(), title.begin(), ::tolower);
        std::string image = p[i]["images"][0]["src"].GetString();
        pc.createProduct(id, title, handle, image);

        // Loop through our variants
        const rapidjson::Value &var = doc["products"][i]["variants"];
        for (rapidjson::SizeType i = 0; i < var.Size(); i++)
        {
          if (!var[i]["available"].GetBool())
          {
            std::string available = "false";
            std::string id2 = std::to_string(var[i]["id"].GetInt64());
            std::string vtitle = var[i]["title"].GetString();
            std::string sku = var[i]["sku"].GetString();
            std::string price = var[i]["price"].GetString();
            pc.addVariant(id, id2, vtitle, sku, price, available);
          }
        }
      }
    }
    std::string response = "";
    Product *pt = pc.findProductByID(productID);
    if (pt != nullptr)
    {
      response += pt->getProduct();
      response += ",";
    }
    delete pt;

    if (response != "")
    {
      response.pop_back();
    }
    response = "[" + response + "]";

    return response;
  }
  else
  {
    return products;
  }
}

std::string Monitor::getSizes(std::string domain, std::string productID)
{
  ProductCollection pc = ProductCollection();
  int i = 0;
  std::string products = Monitor::getAllProducts(domain);
  if (products != "Page not found" && products != "Error")
  {

    rapidjson::Document doc;
    doc.Parse(products.c_str());
    assert(doc["products"].IsArray());
    const rapidjson::Value &p = doc["products"];

    for (rapidjson::SizeType i = 0; i < p.Size(); ++i)
    {
      std::string id = std::to_string(p[i]["id"].GetInt64());
      if (id == productID)
      {

        std::string handle = p[i]["handle"].GetString();
        std::string image = p[i]["images"][0]["src"].GetString();
        std::string title = p[i]["title"].GetString();
        std::transform(title.begin(), title.end(), title.begin(), ::tolower);
        //std::cout << "Found in title: " << title << std::endl;
        //std::cout << "Matched keyword: " << kw << std::endl;
        pc.createProduct(id, title, handle, image);

        // Loop through our variants
        const rapidjson::Value &var = doc["products"][i]["variants"];
        for (rapidjson::SizeType i = 0; i < var.Size(); i++)
        {
          if (var[i]["available"].GetBool())
          {
            std::string available = "true";
            std::string id2 = std::to_string(var[i]["id"].GetInt64());
            std::string vtitle = var[i]["title"].GetString();
            std::string sku = var[i]["sku"].GetString();
            std::string price = var[i]["price"].GetString();
            pc.addVariant(id, id2, vtitle, sku, price, available);
          }
          else
          {
            std::string available = "false";
            std::string id2 = std::to_string(var[i]["id"].GetInt64());
            std::string vtitle = var[i]["title"].GetString();
            std::string sku = var[i]["sku"].GetString();
            std::string price = var[i]["price"].GetString();
            pc.addVariant(id, id2, vtitle, sku, price, available);
          }
        }
      }
    }

    //For each product in product title and product handle eliminate all that dont contain a kw
    std::string response = "";
    Product *pt = pc.findProductByID(productID);
    if (pt != nullptr)
    {
      response += pt->getProduct();
      response += ",";
    }
    delete pt;

    if (response != "")
    {
      response.pop_back();
    }
    response = "[" + response + "]";

    return response;
  }
  else
  {
    return products;
  }
}
std::string Monitor::getRestocked(std::string domain, std::string productID, std::vector<std::string> variants)
{
  ProductCollection pc = ProductCollection();
  int i = 0;
  std::string products = Monitor::getAllProducts(domain);
  if (products != "Page not found" && products != "Error")
  {
    rapidjson::Document doc;
    doc.Parse(products.c_str());
    assert(doc["products"].IsArray());
    const rapidjson::Value &p = doc["products"];
    // Search for product
    // Compare variants

    for (rapidjson::SizeType i = 0; i < p.Size(); ++i)
    {
      std::string id = std::to_string(p[i]["id"].GetInt64());
      if (id == productID)
      {
        std::string handle = p[i]["handle"].GetString();
        std::string image = p[i]["images"][0]["src"].GetString();
        std::string title = p[i]["title"].GetString();
        std::transform(title.begin(), title.end(), title.begin(), ::tolower);
        //std::cout << "Found in title: " << title << std::endl;
        //std::cout << "Matched keyword: " << kw << std::endl;
        pc.createProduct(id, title, handle, image);

        // Loop through our variants
        const rapidjson::Value &var = doc["products"][i]["variants"];
        std::vector<std::string>::iterator iter1;
        for (iter1 = variants.begin(); iter1 != variants.end(); iter1++)
        {
          for (rapidjson::SizeType i = 0; i < var.Size(); i++)
          {
            if (*iter1 == std::to_string(var[i]["id"].GetInt64()))
            {
              if (var[i]["available"].GetBool())
              {
                std::string available = "true";
                std::string id2 = std::to_string(var[i]["id"].GetInt64());
                std::string vtitle = var[i]["title"].GetString();
                std::string sku = var[i]["sku"].GetString();
                std::string price = var[i]["price"].GetString();
                pc.addVariant(id, id2, vtitle, sku, price, available);
              }
            }
          }
        }
      }
    }

    //For each product in product title and product handle eliminate all that dont contain a kw
    std::string response = "[";
    Product *pt = pc.findProductByID(productID);
    if (pt != nullptr)
    {
      response += pt->getProduct();
      response += ",";
    }
    delete pt;

    if (response != "[")
    {
      response.pop_back();
    }
    response += "]";
    return response;
  }
  else
  {
    return products;
  }
}
std::string Monitor::instockSizes(std::string domain, std::string productID)
{
  ProductCollection pc = ProductCollection();
  int i = 0;
  std::string products = Monitor::getAllProducts(domain);
  if (products != "Page not found" && products != "Error")
  {
    rapidjson::Document doc;
    doc.Parse(products.c_str());
    assert(doc["products"].IsArray());
    const rapidjson::Value &p = doc["products"];

    for (rapidjson::SizeType i = 0; i < p.Size(); ++i)
    {
      std::string title = p[i]["title"].GetString();
      std::transform(title.begin(), title.end(), title.begin(), ::tolower);
      std::string id = std::to_string(p[i]["id"].GetInt64());
      std::string handle = p[i]["handle"].GetString();
      std::string image = p[i]["images"][0]["src"].GetString();
      if (id == productID)
      {
        pc.createProduct(id, title, handle, image);

        // Loop through our variants
        const rapidjson::Value &var = doc["products"][i]["variants"];
        for (rapidjson::SizeType i = 0; i < var.Size(); i++)
        {
          if (var[i]["available"].GetBool())
          {
            std::string available = "true";
            std::string id2 = std::to_string(var[i]["id"].GetInt64());
            std::string vtitle = var[i]["title"].GetString();
            std::string sku = var[i]["sku"].GetString();
            std::string price = var[i]["price"].GetString();
            pc.addVariant(id, id2, vtitle, sku, price, available);
          }
        }
      }
    }
    std::string response = "";
    Product *pt = pc.findProductByID(productID);
    if (pt != nullptr)
    {
      response += pt->getProduct();
      response += ",";
    }
    delete pt;

    if (response != "")
    {
      response.pop_back();
    }
    response = "[" + response + "]";

    return response;
  }
  else
  {
    return products;
  }
}
std::string Monitor::findProductByHandle(std::string domain, std::vector<std::string> keywords)
{
  int i = 0;
  ProductCollection pc = ProductCollection();
  std::vector<std::string>::iterator itr;
  std::vector<std::string> foundhandle;
  unique(keywords);
  std::string products = Monitor::getAllProducts(domain);
  if (products != "Page not found" && products != "Error")
  {
    rapidjson::Document doc;
    doc.Parse(products.c_str());
    assert(doc["products"].IsArray());
    const rapidjson::Value &p = doc["products"];
    // === FIND ===
    for (itr = keywords.begin(); itr != keywords.end(); ++itr)
    {
      for (rapidjson::SizeType i = 0; i < p.Size(); ++i)
      {
        std::string kw = *itr;
        std::string handle = p[i]["handle"].GetString();

        std::transform(kw.begin(), kw.end(), kw.begin(), ::tolower);
        std::transform(handle.begin(), handle.end(), handle.begin(), ::tolower);
        if (handle.find(kw) != std::string::npos)
        {
          //BUILD OUR PRODUCT
          std::string title = p[i]["title"].GetString();
          std::string id = std::to_string(p[i]["id"].GetInt64());
          std::string image = p[i]["images"][0]["src"].GetString();
          std::string handle = p[i]["handle"].GetString();
          pc.createProduct(id, title, handle, image);

          // Loop through our variants
          const rapidjson::Value &var = doc["products"][i]["variants"];
          for (rapidjson::SizeType i = 0; i < var.Size(); i++)
          {
            std::string id2 = std::to_string(var[i]["id"].GetInt64());
            std::string vtitle = var[i]["title"].GetString();
            std::string sku = var[i]["sku"].GetString();
            std::string price = var[i]["price"].GetString();
            std::string available;
            if (var[i]["available"].GetBool())
            {
              available = "true";
            }
            else
            {
              available = "false";
            }
            pc.addVariant(id, id2, vtitle, sku, price, available);
          }
          foundhandle.push_back(handle);
        }
      }
    }
    //For each product in product title and product handle eliminate all that dont contain a kw   === FILTER ===
    unique(foundhandle);
    for (itr = keywords.begin(); itr != keywords.end(); ++itr)
    {
      std::string kw = *itr;
      std::vector<std::string>::iterator itr2;
      for (itr2 = foundhandle.begin(); itr2 != foundhandle.end(); ++itr2)
      {
        std::string handle = *itr2;
        if (handle.find(kw) == std::string::npos)
        {
          foundhandle.erase(itr2--);
        }
      }
    }
    std::string response = "";
    for (itr = foundhandle.begin(); itr != foundhandle.end(); ++itr)
    {
      Product *pt = pc.findProductByHandle(*itr);
      if (pt != nullptr)
      {
        response += pt->getProduct();
        response += ",";
      }
      delete pt;
    }
    if (response != "")
    {
      response.pop_back();
    }
    response = "[" + response + "]";
    return response;
  }
  else
  {
    return products;
  }
  //BUILD OUR PRODUCT OBJECT HERE
}

std::string Monitor::searchProductByHandle(std::string product, std::vector<std::string> keywords)
{
  int i = 0;
  ProductCollection pc = ProductCollection();
  std::vector<std::string>::iterator itr;
  std::vector<std::string> foundhandle;
  unique(keywords);
  std::string products = product;
  if (products != "Page not found" && products != "Error")
  {
    rapidjson::Document doc;
    doc.Parse(products.c_str());
    assert(doc["products"].IsArray());
    const rapidjson::Value &p = doc["products"];
    // === FIND ===
    for (itr = keywords.begin(); itr != keywords.end(); ++itr)
    {
      for (rapidjson::SizeType i = 0; i < p.Size(); ++i)
      {
        std::string kw = *itr;
        std::string handle = p[i]["handle"].GetString();

        std::transform(kw.begin(), kw.end(), kw.begin(), ::tolower);
        std::transform(handle.begin(), handle.end(), handle.begin(), ::tolower);
        if (handle.find(kw) != std::string::npos)
        {
          //BUILD OUR PRODUCT
          std::string title = p[i]["title"].GetString();
          std::string id = std::to_string(p[i]["id"].GetInt64());
          std::string image = p[i]["images"][0]["src"].GetString();
          std::string handle = p[i]["handle"].GetString();
          pc.createProduct(id, title, handle, image);

          // Loop through our variants
          const rapidjson::Value &var = doc["products"][i]["variants"];
          for (rapidjson::SizeType i = 0; i < var.Size(); i++)
          {
            std::string id2 = std::to_string(var[i]["id"].GetInt64());
            std::string vtitle = var[i]["title"].GetString();
            std::string sku = var[i]["sku"].GetString();
            std::string price = var[i]["price"].GetString();
            std::string available;
            if (var[i]["available"].GetBool())
            {
              available = "true";
            }
            else
            {
              available = "false";
            }
            pc.addVariant(id, id2, vtitle, sku, price, available);
          }
          foundhandle.push_back(handle);
        }
      }
    }
    //For each product in product title and product handle eliminate all that dont contain a kw   === FILTER ===
    unique(foundhandle);
    for (itr = keywords.begin(); itr != keywords.end(); ++itr)
    {
      std::string kw = *itr;
      std::vector<std::string>::iterator itr2;
      for (itr2 = foundhandle.begin(); itr2 != foundhandle.end(); ++itr2)
      {
        std::string handle = *itr2;
        if (handle.find(kw) == std::string::npos)
        {
          foundhandle.erase(itr2--);
        }
      }
    }
    std::string response = "";
    for (itr = foundhandle.begin(); itr != foundhandle.end(); ++itr)
    {
      Product *pt = pc.findProductByHandle(*itr);
      if (pt != nullptr)
      {
        response += pt->getProduct();
        response += ",";
      }
      delete pt;
    }
    if (response != "")
    {
      response.pop_back();
    }
    response = "[" + response + "]";
    return response;
  }
  else
  {
    return products;
  }
  //BUILD OUR PRODUCT OBJECT HERE
}

std::string Monitor::searchSizes(std::string product, std::string productID)
{
  ProductCollection pc = ProductCollection();
  int i = 0;
  std::string products = product;
  if (products != "Page not found" && products != "Error")
  {

    rapidjson::Document doc;
    doc.Parse(products.c_str());
    assert(doc["products"].IsArray());
    const rapidjson::Value &p = doc["products"];

    for (rapidjson::SizeType i = 0; i < p.Size(); ++i)
    {
      std::string id = std::to_string(p[i]["id"].GetInt64());
      if (id == productID)
      {

        std::string handle = p[i]["handle"].GetString();
        std::string image = p[i]["images"][0]["src"].GetString();
        std::string title = p[i]["title"].GetString();
        std::transform(title.begin(), title.end(), title.begin(), ::tolower);
        //std::cout << "Found in title: " << title << std::endl;
        //std::cout << "Matched keyword: " << kw << std::endl;
        pc.createProduct(id, title, handle, image);

        // Loop through our variants
        const rapidjson::Value &var = doc["products"][i]["variants"];
        for (rapidjson::SizeType i = 0; i < var.Size(); i++)
        {
          if (var[i]["available"].GetBool())
          {
            std::string available = "true";
            std::string id2 = std::to_string(var[i]["id"].GetInt64());
            std::string vtitle = var[i]["title"].GetString();
            std::string sku = var[i]["sku"].GetString();
            std::string price = var[i]["price"].GetString();
            pc.addVariant(id, id2, vtitle, sku, price, available);
          }
          else
          {
            std::string available = "false";
            std::string id2 = std::to_string(var[i]["id"].GetInt64());
            std::string vtitle = var[i]["title"].GetString();
            std::string sku = var[i]["sku"].GetString();
            std::string price = var[i]["price"].GetString();
            pc.addVariant(id, id2, vtitle, sku, price, available);
          }
        }
      }
    }

    //For each product in product title and product handle eliminate all that dont contain a kw
    std::string response = "";
    Product *pt = pc.findProductByID(productID);
    if (pt != nullptr)
    {
      response += pt->getProduct();
      response += ",";
    }
    delete pt;

    if (response != "")
    {
      response.pop_back();
    }
    response = "[" + response + "]";

    return response;
  }
  else
  {
    return products;
  }
}
std::string Monitor::searchOutofstockSizes(std::string product, std::string productID)
{
  ProductCollection pc = ProductCollection();
  int i = 0;
  std::string products = product;
  if (products != "Page not found" && products != "Error")
  {
    rapidjson::Document doc;
    doc.Parse(products.c_str());
    assert(doc["products"].IsArray());
    const rapidjson::Value &p = doc["products"];

    for (rapidjson::SizeType i = 0; i < p.Size(); ++i)
    {
      std::string id = std::to_string(p[i]["id"].GetInt64());
      std::string handle = p[i]["handle"].GetString();
      if (id == productID)
      {
        std::string title = p[i]["title"].GetString();
        std::transform(title.begin(), title.end(), title.begin(), ::tolower);
        std::string image = p[i]["images"][0]["src"].GetString();
        pc.createProduct(id, title, handle, image);

        // Loop through our variants
        const rapidjson::Value &var = doc["products"][i]["variants"];
        for (rapidjson::SizeType i = 0; i < var.Size(); i++)
        {
          if (!var[i]["available"].GetBool())
          {
            std::string available = "false";
            std::string id2 = std::to_string(var[i]["id"].GetInt64());
            std::string vtitle = var[i]["title"].GetString();
            std::string sku = var[i]["sku"].GetString();
            std::string price = var[i]["price"].GetString();
            pc.addVariant(id, id2, vtitle, sku, price, available);
          }
        }
      }
    }
    std::string response = "";
    Product *pt = pc.findProductByID(productID);
    if (pt != nullptr)
    {
      response += pt->getProduct();
      response += ",";
    }
    delete pt;

    if (response != "")
    {
      response.pop_back();
    }
    response = "[" + response + "]";

    return response;
  }
  else
  {
    return products;
  }
}
std::string Monitor::searchInstockSizes(std::string product, std::string productID)
{
  ProductCollection pc = ProductCollection();
  int i = 0;
  std::string products = product;
  if (products != "Page not found" && products != "Error")
  {
    rapidjson::Document doc;
    doc.Parse(products.c_str());
    assert(doc["products"].IsArray());
    const rapidjson::Value &p = doc["products"];

    for (rapidjson::SizeType i = 0; i < p.Size(); ++i)
    {
      std::string title = p[i]["title"].GetString();
      std::transform(title.begin(), title.end(), title.begin(), ::tolower);
      std::string id = std::to_string(p[i]["id"].GetInt64());
      std::string handle = p[i]["handle"].GetString();
      std::string image = p[i]["images"][0]["src"].GetString();
      if (id == productID)
      {
        pc.createProduct(id, title, handle, image);

        // Loop through our variants
        const rapidjson::Value &var = doc["products"][i]["variants"];
        for (rapidjson::SizeType i = 0; i < var.Size(); i++)
        {
          if (var[i]["available"].GetBool())
          {
            std::string available = "true";
            std::string id2 = std::to_string(var[i]["id"].GetInt64());
            std::string vtitle = var[i]["title"].GetString();
            std::string sku = var[i]["sku"].GetString();
            std::string price = var[i]["price"].GetString();
            pc.addVariant(id, id2, vtitle, sku, price, available);
          }
        }
      }
    }
    std::string response = "";
    Product *pt = pc.findProductByID(productID);
    if (pt != nullptr)
    {
      response += pt->getProduct();
      response += ",";
    }
    delete pt;

    if (response != "")
    {
      response.pop_back();
    }
    response = "[" + response + "]";

    return response;
  }
  else
  {
    return products;
  }
}